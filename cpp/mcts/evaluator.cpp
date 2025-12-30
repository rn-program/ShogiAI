#include "mcts/evaluator.hpp"
#include "nn/nn_encoder.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>

namespace mcts
{

    Evaluator::Evaluator(
        NNModel &nn,
        const std::string &engine, 
        bool debug_startup
    )
        : nn_(nn),
          yaneura_(engine, debug_startup)
    {
    }

    EvalResult Evaluator::evaluate(const State &state)
    {
        EvalResult result;

        // -----------------------
        // policy : NN
        // -----------------------
        torch::Tensor x = board_to_tensor(state.board);
        auto [policy_logits, _unused_value] = nn_.forward(x);

        result.policy_logits.resize(policy_logits.size(1));
        for (int i = 0; i < policy_logits.size(1); ++i)
        {
            result.policy_logits[i] =
                policy_logits[0][i].item<double>();
        }

        // -----------------------
        // value : やねうら王(nn.bin)
        // -----------------------
        // SFEN 生成
        std::string sfen = state.toSfen();

        // cp 取得（手番側有利）

        int cp = yaneura_.evaluate(sfen);

        // [-1, 1] に正規化
        // 1000cp ≒ 勝ち確、-1000cp ≒ 負け確
        constexpr double SCALE = 1000.0;
        double v = static_cast<double>(cp) / SCALE;
        result.value = std::clamp(v, -1.0, 1.0);

        return result;
    }

} // namespace mcts
