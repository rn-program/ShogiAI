#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <filesystem>

#include "core/Board.hpp"
#include "nn/nn_model.hpp"

#include "mcts/mcts.hpp"
#include "mcts/policy.hpp"
#include "mcts/evaluator.hpp"

namespace py = pybind11;
namespace fs = std::filesystem;
using namespace shogi;

// =======================
// Python から呼ぶ関数
// =======================
std::string search_move(
    const std::string &model_dir,
    const std::string &sfen,
    int simulations)
{
    // =======================
    // NN 読み込み
    // =======================
    NNModel nn(
        (fs::path(model_dir) / "policy_net.ts").string(),
        torch::kCPU);

    std::string engine = "path/to/engine"; // ここにエンジンパスを記述

    // =======================
    // move_dicts 読み込み
    // =======================
    std::unordered_map<int, std::string> idx2move;
    std::unordered_map<std::string, int> move2idx;

    {
        std::ifstream fin(fs::path(model_dir) / "move_dicts.txt");
        if (!fin)
            throw std::runtime_error("Failed to open move_dicts.txt");

        int idx;
        std::string usi;
        while (fin >> idx >> usi)
        {
            idx2move[idx] = usi;
            move2idx[usi] = idx;
        }
    }

    // =======================
    // 初期局面
    // =======================
    Board board;
    board.setPositionFromSFEN(sfen);

    // =======================
    // MCTS 構成
    // =======================
    mcts::State root_state(board);
    mcts::Policy policy(idx2move, move2idx);
    mcts::Evaluator evaluator(nn, engine, true);

    mcts::MCTS mcts(
        policy,
        evaluator,
        simulations);

    // =======================
    // 探索
    // =======================
    return mcts.search(root_state, 0.0);
}

// =======================
// pybind11 module
// =======================
PYBIND11_MODULE(nukocat_cpp, m)
{
    m.doc() = "NukoCat MCTS module";

    m.def(
        "search",
        &search_move,
        py::arg("model_dir"),
        py::arg("sfen"),
        py::arg("simulations") = 800,
        "Run MCTS search and return best USI move");
}
