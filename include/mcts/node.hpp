// node.hpp
#pragma once
#include <unordered_map>
#include <memory>
#include <string>

namespace mcts
{

    struct Node
    {
        // MCTS statistics
        int N = 0;      // visit count
        double W = 0.0; // total value
        double Q = 0.0; // mean value
        double P = 0.0; // prior probability

        // children: usi -> Node
        std::unordered_map<std::string, std::unique_ptr<Node>> children;

        Node() = default;           // ★ 必須
        explicit Node(double prior) // 子ノード用
            : P(prior)
        {
        }
    };

} // namespace mcts
