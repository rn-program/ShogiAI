#pragma once

#include <vector>

// NN
#include "nn/nn_model.hpp"

// State
#include "state.hpp"

// YaneuraOu Engine
#include "YaneuraOu/YaneuraOuEngine.hpp"

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
        // nn     : policy 用 NN
        // engine : 起動済みのやねうら王エンジン（共有）
        Evaluator(
            NNModel &nn,
            YaneuraOuEngine &engine);

        // State を評価して policy + value を返す
        EvalResult evaluate(const State &state);

    private:
        NNModel &nn_;
        YaneuraOuEngine &engine_; // 所有しない（参照）
    };

} // namespace mcts
