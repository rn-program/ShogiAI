#include "state.hpp"

namespace mcts
{

    void State::apply(const std::string &usi)
    {
        // Board が USI を直接適用できるという前提
        board.applyUSI(usi);
    }

} // namespace mcts
