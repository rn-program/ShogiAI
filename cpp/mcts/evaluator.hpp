#pragma once

#include <vector>

// NN
#include "../nn/nn_model.hpp"

// State
#include "state.hpp"

namespace mcts
{

    // =======================
    // 評価結果
    // =======================
    struct EvalResult
    {
        std::vector<double> policy_logits; // NN の生 logits
        double value;                      // [-1, 1]
    };

    // =======================
    // Evaluator
    // =======================
    class Evaluator
    {
    public:
        explicit Evaluator(NNModel &nn);

        // State を評価して policy + value を返す
        EvalResult evaluate(const State &state);

    private:
        NNModel &nn_;
    };

} // namespace mcts
