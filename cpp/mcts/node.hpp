// node.hpp
#pragma once
#include <unordered_map>
#include <memory>
#include "state.hpp"
#include "edge.hpp"

namespace mcts
{

    struct Node
    {
        State state;
        int visits = 0;
        bool expanded = false;

        std::unordered_map<std::string, std::unique_ptr<Edge>> edges;

        explicit Node(const State &s) : state(s) {}
    };

} // namespace mcts
