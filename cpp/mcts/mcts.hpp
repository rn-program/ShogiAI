// mcts.hpp
#pragma once
#include "node.hpp"
#include "policy.hpp"
#include "evaluator.hpp"
#include "config.hpp"

namespace mcts {

class MCTS {
public:
    MCTS(Evaluator& eval, Policy& policy, const Config& cfg);

    std::string search(const shogi::Board& board);

private:
    double simulate(Node* node);

    Evaluator& eval_;
    Policy& policy_;
    Config cfg_;
};

} // namespace mcts
