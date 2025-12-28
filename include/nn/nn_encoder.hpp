#pragma once
#include <torch/torch.h>
#include "core/Board.hpp"

using namespace shogi;

torch::Tensor board_to_tensor(const shogi::Board& board);
