#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <torch/script.h>
#include <filesystem>
#include <cmath>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#include "../core/Board.hpp"
#include "../nn/nn_encoder.hpp" // ★ board_to_tensor
#include "../nn/nn_model.hpp"   // ★ NNModel

namespace py = pybind11;
namespace fs = std::filesystem;
using namespace shogi;

// =======================
// Node
// =======================
struct Node
{
    int visits = 0;
    double value_sum = 0.0;
    double value_mean = 0.0;
    double prior = 0.0;
    std::unordered_map<std::string, std::unique_ptr<Node>> children;
};

// =======================
// PUCT
// =======================
Node *select_child(Node *node, std::string &best_usi)
{
    constexpr double c_puct = 1.5;
    double best_score = -1e18;

    for (auto &[usi, child] : node->children)
    {
        double Q = (child->visits == 0) ? 0.0 : child->value_mean;
        double U = c_puct * child->prior *
                   std::sqrt(node->visits + 1) / (1 + child->visits);
        double score = Q + U;

        if (score > best_score)
        {
            best_score = score;
            best_usi = usi;
        }
    }
    return node->children.at(best_usi).get();
}

// =======================
// Expand
// =======================
void expand_node(
    Node *node,
    Board &board,
    NNModel &nn,
    const std::unordered_map<int, std::string> &idx2move,
    const std::unordered_map<std::string, int> &move2idx)
{
    const int num_moves = idx2move.size();

    // ---------- legal mask ----------
    std::vector<bool> legal_mask(num_moves, false);
    auto legal_moves = board.generateLegalMoves(board.turn);

    for (auto &m : legal_moves)
    {
        std::string usi = m.toUSI(board.turn);
        auto it = move2idx.find(usi);
        if (it != move2idx.end())
            legal_mask[it->second] = true;
    }

    // ---------- NN ----------
    torch::Tensor x = board_to_tensor(board);
    auto [policy_logits, value] = nn.forward(x);

    // ---------- expand ----------
    double sum_p = 0.0;

    for (int idx = 0; idx < num_moves; ++idx)
    {
        if (!legal_mask[idx])
            continue;

        double p = std::exp(policy_logits[0][idx].item<double>());
        if (p <= 0.0)
            continue;

        auto child = std::make_unique<Node>();
        child->prior = p;

        const std::string &usi = idx2move.at(idx);
        node->children.emplace(usi, std::move(child));
        sum_p += p;
    }

    // ---------- normalize ----------
    if (sum_p > 0.0)
    {
        for (auto &[_, child] : node->children)
            child->prior /= sum_p;
    }
}

// =======================
// Simulation
// =======================
double simulate(
    Node *node,
    Board board,
    NNModel &nn,
    const std::unordered_map<int, std::string> &idx2move,
    const std::unordered_map<std::string, int> &move2idx)
{
    // leaf
    if (node->children.empty())
    {
        expand_node(node, board, nn, idx2move, move2idx);

        torch::Tensor x = board_to_tensor(board);
        auto [_, value] = nn.forward(x);

        return value.item<double>();
    }

    // select
    std::string usi;
    Node *child = select_child(node, usi);

    board.applyUSI(usi);

    // recursion (手番反転)
    double value = -simulate(child, board, nn, idx2move, move2idx);

    child->visits += 1;
    child->value_sum += value;
    child->value_mean = child->value_sum / child->visits;

    return value;
}

// =======================
// Search
// =======================
Move mcts_search(
    Board board,
    NNModel &nn,
    const std::unordered_map<int, std::string> &idx2move,
    const std::unordered_map<std::string, int> &move2idx,
    int simulations)
{
    Node root;

    for (int i = 0; i < simulations; ++i)
    {
        Board tmp = board;
        simulate(&root, tmp, nn, idx2move, move2idx);
        root.visits++;
    }

    // visits 最大
    std::string best_usi;
    int best_visits = -1;

    for (auto &[usi, child] : root.children)
    {
        if (child->visits > best_visits)
        {
            best_visits = child->visits;
            best_usi = usi;
        }
    }

    std::cout << "[MCTS] best = " << best_usi
              << " visits=" << best_visits << std::endl;

    return board.fromUSI(best_usi, board.turn);
}

// =======================
// pybind
// =======================
std::string predict_move(
    const std::string &project_root,
    const std::string &username,
    const std::string &sfen,
    int simulations)
{
    fs::path base = fs::path(project_root) / "trained_models" / username;

    NNModel nn((base / "policy_value.ts").string(), torch::kCPU);

    std::unordered_map<int, std::string> idx2move;
    std::unordered_map<std::string, int> move2idx;

    {
        std::ifstream fin(base / "move_dicts.txt");
        int idx;
        std::string usi;
        while (fin >> idx >> usi)
        {
            idx2move[idx] = usi;
            move2idx[usi] = idx;
        }
    }

    Board board;
    board.setPositionFromSFEN(sfen);

    Move best = mcts_search(board, nn, idx2move, move2idx, simulations);
    return best.toUSI(board.turn);
}

PYBIND11_MODULE(nukocat_cpp, m)
{
    m.def(
        "predict_move",
        &predict_move,
        py::arg("project_root"),
        py::arg("username"),
        py::arg("sfen"),
        py::arg("simulations") = 800);
}
