#include "mcts.hpp"

#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>

#include "../nn/nn_encoder.hpp"

namespace shogi {

// =======================
// MCTS Node
// =======================
struct MCTSNode {
    Board board;
    Move move_from_parent;
    MCTSNode *parent;
    std::vector<std::unique_ptr<MCTSNode>> children;

    int visits = 0;
    float value_sum = 0.0f;

    float prior = 0.0f;   // policy P
    bool expanded = false;

    MCTSNode(const Board &b, MCTSNode *p = nullptr, Move m = Move{})
        : board(b), move_from_parent(m), parent(p) {}

    float Q() const {
        return visits == 0 ? 0.0f : value_sum / visits;
    }

    float PUCT(float c_puct = 1.0f) const {
        float Np = parent ? parent->visits : 1;
        return Q() + c_puct * prior * std::sqrt(Np) / (1 + visits);
    }
};

// =======================
// Selection
// =======================
MCTSNode* select(MCTSNode *node) {
    while (node->expanded && !node->children.empty()) {
        node = std::max_element(
            node->children.begin(),
            node->children.end(),
            [](const auto &a, const auto &b) {
                return a->PUCT() < b->PUCT();
            }
        )->get();
    }
    return node;
}

// =======================
// Expansion + Evaluation
// =======================
float expand_and_evaluate(
    MCTSNode *node,
    torch::jit::script::Module &model,
    const std::unordered_map<int, std::string> &idx2move
) {
    // NN forward
    torch::Tensor x = board_to_tensor(node->board);
    auto outputs = model.forward({x}).toTuple();

    torch::Tensor policy_logits = outputs->elements()[0].toTensor();
    torch::Tensor value = outputs->elements()[1].toTensor();

    // 合法手
    auto legal_moves = node->board.generateLegalMoves(node->board.turn);

    // mask
    torch::Tensor mask =
        torch::zeros({(long)idx2move.size()}, torch::kBool);

    for (const auto &m : legal_moves) {
        for (const auto &[idx, usi] : idx2move) {
            if (usi == m.toUSI(node->board.turn)) {
                mask[idx] = true;
                break;
            }
        }
    }

    policy_logits = policy_logits.squeeze(0);
    policy_logits = policy_logits.masked_fill(~mask, -1e9);
    torch::Tensor policy = torch::softmax(policy_logits, 0);

    // 子ノード生成
    for (const auto &m : legal_moves) {
        Board next = node->board;
        next.applyMove(const_cast<Move&>(m));

        auto child = std::make_unique<MCTSNode>(next, node, m);

        for (const auto &[idx, usi] : idx2move) {
            if (usi == m.toUSI(node->board.turn)) {
                child->prior = policy[idx].item<float>();
                break;
            }
        }

        node->children.push_back(std::move(child));
    }

    node->expanded = true;

    // value head
    return value.item<float>();
}

// =======================
// Backpropagation
// =======================
void backpropagate(MCTSNode *node, float value) {
    while (node) {
        node->visits++;
        node->value_sum += value;
        value = -value; // 手番交代
        node = node->parent;
    }
}

// =======================
// MCTS main
// =======================
Move mcts_search(
    Board root_board,
    torch::jit::script::Module &model,
    const std::unordered_map<int, std::string> &idx2move,
    int simulations
) {
    MCTSNode root(root_board);

    for (int i = 0; i < simulations; ++i) {
        MCTSNode *node = select(&root);

        float value;
        if (!node->expanded) {
            value = expand_and_evaluate(node, model, idx2move);
        } else {
            value = node->Q();
        }

        backpropagate(node, value);
    }

    // 最多訪問手
    auto best = std::max_element(
        root.children.begin(),
        root.children.end(),
        [](const auto &a, const auto &b) {
            return a->visits < b->visits;
        }
    );

    return (*best)->move_from_parent;
}

}
