#pragma once

#include <unordered_map>
#include <vector>
#include <string>

#include "../core/Board.hpp" // shogi::Board

namespace mcts
{

    class Policy
    {
    public:
        Policy(
            const std::unordered_map<int, std::string> &idx2move,
            const std::unordered_map<std::string, int> &move2idx);

        // 合法手（USI文字列）を取得
        std::vector<std::string>
        legal_usis(const shogi::Board &board) const;

        // 合法手でマスクした policy
        std::vector<double>
        masked_policy(
            const std::vector<double> &logits,
            const std::vector<std::string> &legal) const;

    private:
        std::unordered_map<int, std::string> idx2move_;
        std::unordered_map<std::string, int> move2idx_;
    };

} // namespace mcts
