#pragma once

#include "../core/Board.hpp"
#include <string>

namespace mcts
{

    class State
    {
    public:
        explicit State(const shogi::Board &board)
            : board(board) {}

        void apply(const std::string &usi);

        shogi::Board board;
    };

} // namespace mcts
