#pragma once

#include <vector>
#include <string>

// NN
#include "nn/nn_model.hpp"

// State
#include "state.hpp"

// YaneuraOu (USI wrapper)
#include "YaneuraOu/YaneuraOu.hpp"

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
        // nn      : policy 用 NN
        // engine  : やねうら王の実行ファイルパス
        Evaluator(NNModel &nn, const std::string &engine, bool debug_startup);

        // State を評価して policy + value を返す
        EvalResult evaluate(const State &state);

    private:
        NNModel &nn_;
        YaneuraOuValue yaneura_;
    };

} // namespace mcts
