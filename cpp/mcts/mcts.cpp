#include "mcts.hpp"

#include <algorithm>
#include <cmath>
#include <random>

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
            add_dirichlet_noise(root);
        }

        for (int i = 0; i < simulations_; ++i)
        {
            State s = root_state;
            simulate(root, s);
        }

        // temperature 付きサンプリング
        std::vector<std::string> moves;
        std::vector<double> weights;

        for (auto &[usi, child] : root.children)
        {
            moves.push_back(usi);
            weights.push_back(std::pow(child->N, 1.0 / temperature));
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
            double v = state.terminal_value();
            node.N += 1;
            node.W += v;
            node.Q = node.W / node.N;
            return v;
        }

        // 未展開
        if (node.children.empty())
        {
            auto eval = evaluator_.evaluate(state);
            auto legal = policy_.legal_usis(state.board);
            auto probs = policy_.masked_policy(eval.policy_logits, legal);

            for (size_t i = 0; i < legal.size(); ++i)
            {
                auto child = std::make_unique<Node>();
                child->P = probs[i];
                node.children.emplace(legal[i], std::move(child));
            }

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
                std::sqrt(node.N + 1.0) / (1.0 + child->N);

            double score = child->Q + U;
            if (score > best_score)
            {
                best_score = score;
                best = child.get();
                best_usi = usi;
            }
        }

        state.apply(best_usi);
        double value = -simulate(*best, state);

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
