#include "policy.hpp"
#include <cmath>

namespace mcts
{

    Policy::Policy(
        const std::unordered_map<int, std::string> &i2m,
        const std::unordered_map<std::string, int> &m2i)
        : idx2move_(i2m), move2idx_(m2i) {}

    std::vector<std::string>
    Policy::legal_usis(const shogi::Board &board) const
    {
        std::vector<std::string> res;
        auto moves = board.generateLegalMoves(board.turn);
        for (auto &m : moves)
            res.push_back(m.toUSI(board.turn));
        return res;
    }

    std::vector<double>
    Policy::masked_policy(
        const std::vector<double> &logits,
        const std::vector<std::string> &legal) const
    {

        std::vector<double> probs(legal.size(), 1.0);

        double sum = 0.0;
        for (size_t i = 0; i < legal.size(); ++i)
        {
            auto it = move2idx_.find(legal[i]);
            double p = (it == move2idx_.end())
                           ? 1.0
                           : std::exp(logits[it->second]);
            probs[i] = p;
            sum += p;
        }

        for (double &p : probs)
            p /= sum;
        return probs;
    }

} // namespace mcts
