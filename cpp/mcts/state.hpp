// state.hpp
#pragma once
#include "../core/Board.hpp"
#include "../core/Player.hpp"

namespace mcts {

struct State {
    shogi::Board board;
    Player turn;

    State(const shogi::Board& b)
        : board(b), turn(b.turn) {}
};

} // namespace mcts
