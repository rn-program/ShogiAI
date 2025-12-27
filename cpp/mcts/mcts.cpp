
#include "mcts.hpp"

#include <algorithm>
#include <cmath>
#include <random>

// child

namespace mcts
{

    static std::mt19937 rng(std::random_device{}());

    // -----------------------
    // コンストラクタ
    // -----------------------
    MCTS::MCTS(
        Policy &policy,
        Evaluator &evaluator,
        int simulations,
        double c_puct,
        double dirichlet_alpha,
        double dirichlet_eps)
        : policy_(policy),
          evaluator_(evaluator),
          simulations_(simulations),
          c_puct_(c_puct),
          dirichlet_alpha_(dirichlet_alpha),
          dirichlet_eps_(dirichlet_eps)
    {
    }

    // -----------------------
    // 探索エントリ（学習用）
    // -----------------------
    std::string MCTS::search(const State &root_state, double temperature)
    {
        Node root;

        // root を一度展開
        {
            State s = root_state;
            simulate(root, s);
        }

        // 本探索
        for (int i = 0; i < simulations_; ++i)
        {
            State s = root_state;
            simulate(root, s);
        }

        // temperature 付き行動選択（visit 数ベース）
        std::vector<std::string> moves;
        std::vector<double> weights;

        for (auto &[usi, child] : root.children)
        {
            moves.push_back(usi);
            weights.push_back(
                temperature <= 0.0
                    ? static_cast<double>(child->N)
                    : std::pow(child->N, 1.0 / temperature));
        }

        // temperature = 0 の場合は最大 visit
        if (temperature <= 0.0)
        {
            auto it = std::max_element(weights.begin(), weights.end());
            return moves[std::distance(weights.begin(), it)];
        }

        std::discrete_distribution<> dist(weights.begin(), weights.end());
        return moves[dist(rng)];
    }

    // -----------------------
    // 再帰シミュレーション
    // -----------------------
    double MCTS::simulate(Node &node, State &state)
    {
        // 終端
        if (state.is_terminal())
        {
            return state.terminal_value();
        }

        // 未展開ノード：展開のみ行い、統計は更新しない
        if (node.children.empty())
        {
            auto eval = evaluator_.evaluate(state);
            auto legal = policy_.legal_usis(state.board);
            auto probs = policy_.masked_policy(eval.policy_logits, legal);

            for (size_t i = 0; i < legal.size(); ++i)
            {
                if (probs[i] <= 0.0)
                    continue; // ★ policy に無い手はノード自体作らない

                auto child = std::make_unique<Node>();
                child->P = probs[i];
                node.children.emplace(legal[i], std::move(child));
            }

            assert(!node.children.empty());

            node.N += 1;
            node.W += eval.value;
            node.Q = node.W / node.N;
            return eval.value;
        }

        // Selection (PUCT)
        Node *best = nullptr;
        std::string best_usi;
        double best_score = -1e18;

        for (auto &[usi, child] : node.children)
        {
            double U =
                c_puct_ * child->P *
                std::sqrt(static_cast<double>(node.N) + 1.0) /
                (1.0 + static_cast<double>(child->N));

            double score = child->Q + U;
            if (score > best_score)
            {
                best_score = score;
                best = child.get();
                best_usi = usi;
            }
        }

        // 1 手進める
        State next = state.apply(best_usi);

        // 再帰（手番反転）
        double value = -simulate(*best, next);

        // Backup（ここだけで統計更新）
        best->N += 1;
        best->W += value;
        best->Q = best->W / best->N;

        node.N += 1;
        node.W += value;
        node.Q = node.W / node.N;

        return value;
    }

    // -----------------------
    // Dirichlet noise（root専用）
    // -----------------------
    void MCTS::add_dirichlet_noise(Node &root)
    {
        if (root.children.empty())
            return;

        std::gamma_distribution<double> gamma(dirichlet_alpha_, 1.0);

        std::vector<double> noise;
        double sum = 0.0;

        for (size_t i = 0; i < root.children.size(); ++i)
        {
            double n = gamma(rng);
            noise.push_back(n);
            sum += n;
        }

        size_t i = 0;
        for (auto &[_, child] : root.children)
        {
            double eta = noise[i++] / sum;
            child->P =
                (1.0 - dirichlet_eps_) * child->P +
                dirichlet_eps_ * eta;
        }
    }

} // namespace mcts
