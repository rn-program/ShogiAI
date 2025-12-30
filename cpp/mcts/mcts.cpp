#include "mcts/mcts.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <cassert>

namespace mcts
{
    static std::mt19937 rng(std::random_device{}());

    MCTS::MCTS(
        Policy &policy,
        Evaluator &evaluator,
        int simulations,
        double c_puct,
        double dirichlet_alpha,
        double dirichlet_eps,
        YaneuraOuEngine *engine)
        : policy_(policy),
          evaluator_(evaluator),
          simulations_(simulations),
          c_puct_(c_puct),
          dirichlet_alpha_(dirichlet_alpha),
          dirichlet_eps_(dirichlet_eps),
          engine_(engine)
    {
    }

    std::string MCTS::search(const State &root_state, double temperature)
    {
        Node root;

        State s = root_state;
        auto eval = evaluator_.evaluate(s);

        // engine がある場合は NN value と比較
        if (engine_)
        {
            double engine_value = engine_->evaluatePosition(s.toSfen());
            if (std::abs(eval.value - engine_value) > 0.8)
            {
                // root prior 補正
                auto legal = policy_.legal_usis(s.board);
                for (const auto &usi : legal)
                {
                    auto child = std::make_unique<Node>();
                    child->P = 0.7 * engine_value + 0.3 * eval.value;
                    root.children.emplace(usi, std::move(child));
                }
            }
        }

        simulate(root, s);
        add_dirichlet_noise(root);

        for (int i = 0; i < simulations_; ++i)
        {
            State s = root_state;
            simulate(root, s);
        }

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

        if (temperature <= 0.0)
        {
            auto it = std::max_element(weights.begin(), weights.end());
            return moves[std::distance(weights.begin(), it)];
        }

        std::discrete_distribution<> dist(weights.begin(), weights.end());
        return moves[dist(rng)];
    }

    double MCTS::simulate(Node &node, State &state)
    {
        if (state.is_terminal())
        {
            return state.terminal_value();
        }

        if (node.children.empty())
        {
            auto eval = evaluator_.evaluate(state);
            auto legal = policy_.legal_usis(state.board);
            auto probs = policy_.masked_policy(eval.policy_logits, legal);

            for (size_t i = 0; i < legal.size(); ++i)
            {
                if (probs[i] <= 0.0)
                    continue;
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

        Node *best = nullptr;
        std::string best_usi;
        double best_score = -1e18;

        for (auto &[usi, child] : node.children)
        {
            double U = c_puct_ * child->P *
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

        State next = state.apply(best_usi);
        double value = -simulate(*best, next);

        best->N += 1;
        best->W += value;
        best->Q = best->W / best->N;

        node.N += 1;
        node.W += value;
        node.Q = node.W / node.N;

        return value;
    }

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
            child->P = (1.0 - dirichlet_eps_) * child->P + dirichlet_eps_ * eta;
        }
    }

} // namespace mcts
