#pragma once

#include "core/Board.hpp"
#include <string>

namespace mcts
{

    class State
    {
    public:
        explicit State(const shogi::Board &board)
            : board(board) {}

        // Board は const なので、新しい State を返す
        State apply(const std::string &usi) const;

        // 終端判定
        bool is_terminal() const;

        // 終端価値（手番視点）
        double terminal_value() const;

        const shogi::Board board;
    };

} // namespace mcts
