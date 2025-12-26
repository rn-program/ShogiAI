#pragma once
#include <string>
#include "policy.hpp"
#include "evaluator.hpp"
#include "state.hpp"
#include "node.hpp"

namespace mcts
{

    class MCTS
    {
    public:
        MCTS(
            Policy &policy,
            Evaluator &evaluator,
            int simulations);

        std::string search(const State &root_state);

    private:
        double simulate(Node &node, State &state);

        Policy &policy_;
        Evaluator &evaluator_;
        int simulations_;

        // ★ 追加
        const double c_puct = 1.5;
    };

} // namespace mcts
