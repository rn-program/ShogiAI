#include "mcts/state.hpp"

namespace mcts
{

State State::apply(const std::string& usi) const
{
    shogi::Board next = board;   // コピー
    next.applyUSI(usi);
    return State(next);
}

bool State::is_terminal() const
{
    // 詰み
    if (board.isCheckMate())
        return true;

    // 合法手が無い場合
    auto moves = board.generateLegalMoves(board.turn);
    if (moves.empty())
        return true;

    return false;
}

double State::terminal_value() const
{
    // is_terminal() == true のときのみ呼ばれる前提

    // 手番側が詰んでいる → 負け
    if (board.isCheckMate())
        return -1.0;

    auto moves = board.generateLegalMoves(board.turn);
    if (moves.empty())
        return -1.0;

    return 0.0;
}

} // namespace mcts
