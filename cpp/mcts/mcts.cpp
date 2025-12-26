#include "mcts.hpp"
#include "node.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace mcts
{

    // =======================
    // コンストラクタ
    // =======================
    MCTS::MCTS(
        Policy &policy,
        Evaluator &evaluator,
        int simulations)
        : policy_(policy),
          evaluator_(evaluator),
          simulations_(simulations)
    {
    }

    // =======================
    // MCTS 探索（エントリポイント）
    // =======================
    std::string MCTS::search(const State &root_state)
    {
        Node root;

        for (int i = 0; i < simulations_; ++i)
        {
            State state = root_state;
            simulate(root, state);
        }

        // 最も visit 数が多い手を返す
        int best_N = -1;
        std::string best_move;

        for (auto &[usi, child] : root.children)
        {
            if (child->N > best_N)
            {
                best_N = child->N;
                best_move = usi;
            }
        }

        return best_move;
    }

    double MCTS::simulate(Node &node, State &state)
    {
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
            return eval.value;
        }

        // selection
        Node *best = nullptr;
        std::string best_usi;
        double best_score = -1e18;

        for (auto &[usi, child] : node.children)
        {
            double U = c_puct * child->P *
                       std::sqrt(node.N + 1) / (1 + child->N);
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

        return value;
    }

} // namespace mcts
