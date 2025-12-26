// mcts.cpp
#include "mcts.hpp"
#include "evaluator.hpp"
#include <cmath>

namespace mcts
{

    MCTS::MCTS(Evaluator &e, Policy &p, const Config &c)
        : eval_(e), policy_(p), cfg_(c) {}

    double MCTS::simulate(Node *node)
    {
        if (!node->expanded)
        {
            auto eval = eval_.evaluate(node->state);
            auto legal = policy_.legal_usis(node->state.board);
            auto probs = policy_.masked_policy(eval.policy, legal);

            for (size_t i = 0; i < legal.size(); ++i)
            {
                auto edge = std::make_unique<Edge>();
                edge->usi = legal[i];
                edge->prior = probs[i];

                shogi::Board next = node->state.board;
                next.applyUSI(legal[i]);

                edge->child = std::make_unique<Node>(State(next));
                node->edges[legal[i]] = std::move(edge);
            }

            node->expanded = true;
            return eval.value;
        }

        // select
        Edge *best = nullptr;
        double best_score = -1e18;

        for (auto &[_, e] : node->edges)
        {
            double U = cfg_.c_puct * e->prior *
                       std::sqrt(node->visits + 1) / (1 + e->visits);
            double score = e->Q() + U;
            if (score > best_score)
            {
                best_score = score;
                best = e.get();
            }
        }

        double value = -simulate(best->child.get());

        best->visits++;
        best->value_sum += value;
        node->visits++;

        return value;
    }

    std::string MCTS::search(const shogi::Board &board)
    {
        Node root(State(board));

        for (int i = 0; i < cfg_.simulations; ++i)
            simulate(&root);

        std::string best;
        int best_v = -1;

        for (auto &[usi, e] : root.edges)
        {
            if (e->visits > best_v)
            {
                best_v = e->visits;
                best = usi;
            }
        }
        return best;
    }

} // namespace mcts
