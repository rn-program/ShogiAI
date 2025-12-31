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

#include "YaneuraOu/YaneuraOuEngine.hpp"

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
    std::string engine_path = ""; // ここにエンジンパスを記入

    std::cerr << "[DEBUG] search_move() entered" << std::endl;
    std::cerr << "[DEBUG] model_dir = " << model_dir << std::endl;
    std::cerr << "[DEBUG] simulations = " << simulations << std::endl;

    // =======================
    // NN 読み込み（1回だけ）
    // =======================
    std::cerr << "[DEBUG] before NNModel static init" << std::endl;

    static NNModel nn(
        (fs::path(model_dir) / "policy_net.ts").string(),
        torch::kCPU);

    std::cerr << "[DEBUG] NNModel initialized" << std::endl;

    // =======================
    // やねうら王エンジン（1回だけ起動）
    // =======================
    std::cerr << "[DEBUG] before YaneuraOuEngine::instance()" << std::endl;
    std::cerr << "[DEBUG] engine path = " << engine_path << std::endl;

    YaneuraOuEngine &engine = YaneuraOuEngine::instance(
        engine_path);

    std::cerr << "[DEBUG] YaneuraOuEngine instance acquired" << std::endl;

    // =======================
    // move_dicts 読み込み
    // =======================
    std::cerr << "[DEBUG] loading move_dicts.txt" << std::endl;

    std::unordered_map<int, std::string> idx2move;
    std::unordered_map<std::string, int> move2idx;

    {
        std::ifstream fin(fs::path(model_dir) / "move_dicts.txt");
        if (!fin)
        {
            std::cerr << "[ERROR] Failed to open move_dicts.txt" << std::endl;
            throw std::runtime_error("Failed to open move_dicts.txt");
        }

        int idx;
        std::string usi;
        while (fin >> idx >> usi)
        {
            idx2move[idx] = usi;
            move2idx[usi] = idx;
        }
    }

    std::cerr << "[DEBUG] move_dicts loaded, size = "
              << idx2move.size() << std::endl;

    // =======================
    // 初期局面
    // =======================
    std::cerr << "[DEBUG] setting board position" << std::endl;

    Board board;
    board.setPositionFromSFEN(sfen);

    std::cerr << "[DEBUG] board initialized" << std::endl;

    // =======================
    // MCTS 構成
    // =======================
    std::cerr << "[DEBUG] constructing MCTS components" << std::endl;

    mcts::State root_state(board);
    mcts::Policy policy(idx2move, move2idx);
    mcts::Evaluator evaluator(nn, engine);

    std::cerr << "[DEBUG] evaluator constructed" << std::endl;

    mcts::MCTS mcts(
        policy,
        evaluator,
        simulations);

    std::cerr << "[DEBUG] MCTS constructed" << std::endl;

    // =======================
    // 探索
    // =======================
    std::cerr << "[DEBUG] starting MCTS search" << std::endl;

    std::string result = mcts.search(root_state, 0.0);

    std::cerr << "[DEBUG] MCTS finished, result = " << result << std::endl;

    return result;
}

// =======================
// pybind11 module
// =======================
PYBIND11_MODULE(nukocat_cpp, m)
{
    std::cerr << "[DEBUG] PYBIND11_MODULE nukocat_cpp loaded" << std::endl;

    m.doc() = "NukoCat MCTS module";

    m.def(
        "search",
        &search_move,
        py::arg("model_dir"),
        py::arg("sfen"),
        py::arg("simulations") = 800,
        "Run MCTS search and return best USI move");
}
