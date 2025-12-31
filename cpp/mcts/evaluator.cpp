#include "mcts/evaluator.hpp"
#include "nn/nn_encoder.hpp"

#include <algorithm>

namespace mcts
{

    Evaluator::Evaluator(
        NNModel &nn,
        YaneuraOuEngine &engine // ← 参照で受け取る
        )
        : nn_(nn),
          engine_(engine)
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
        // value : やねうら王 (nn.bin)
        // -----------------------
        std::string sfen = state.toSfen();

        int cp = engine_.evaluate(sfen);

        constexpr double SCALE = 1000.0;
        double v = static_cast<double>(cp) / SCALE;
        result.value = std::clamp(v, -1.0, 1.0);

        return result;
    }

} // namespace mcts
