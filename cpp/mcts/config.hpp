#pragma once

namespace mcts
{

    struct Config
    {
        int simulations = 800;
        double c_puct = 1.5;
    };

} // namespace mcts
