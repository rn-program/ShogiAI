#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

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
            int simulations,
            double c_puct = 1.5,
            double dirichlet_alpha = 0.3,
            double dirichlet_eps = 0.25);

        // temperature を指定（学習用）
        std::string search(const State &root_state, double temperature);

    private:
        double simulate(Node &node, State &state);
        void add_dirichlet_noise(Node &root);

        Policy &policy_;
        Evaluator &evaluator_;
        int simulations_;

        double c_puct_;
        double dirichlet_alpha_;
        double dirichlet_eps_;
    };

} // namespace mcts
