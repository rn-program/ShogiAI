#pragma once

#include <torch/script.h>
#include <unordered_map>
#include "../core/Board.hpp"
#include "../core/Move.hpp"

namespace shogi {

Move mcts_search(
    Board root_board,
    torch::jit::script::Module &model,
    const std::unordered_map<int, std::string> &idx2move,
    int simulations
);

}
