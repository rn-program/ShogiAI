// edge.hpp
#pragma once
#include <memory>
#include <string>

namespace mcts
{

    struct Node;

    struct Edge
    {
        std::string usi;
        double prior = 0.0;
        int visits = 0;
        double value_sum = 0.0;

        std::unique_ptr<Node> child;

        double Q() const
        {
            return visits == 0 ? 0.0 : value_sum / visits;
        }
    };

} // namespace mcts
