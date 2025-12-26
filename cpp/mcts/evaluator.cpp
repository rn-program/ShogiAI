#include "evaluator.hpp"
#include "../nn/nn_encoder.hpp"
#include <vector>

namespace mcts
{

    Evaluator::Evaluator(NNModel &nn) : nn_(nn) {}

    EvalResult Evaluator::evaluate(const State &state)
    {
        torch::Tensor x = board_to_tensor(state.board);
        auto [policy_logits, value] = nn_.forward(x);

        std::vector<double> logits(policy_logits.size(1));
        for (int i = 0; i < policy_logits.size(1); ++i)
            logits[i] = policy_logits[0][i].item<double>();

        return {logits, value.item<double>()};
    }

} // namespace mcts
