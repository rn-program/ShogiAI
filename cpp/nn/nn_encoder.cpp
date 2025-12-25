#include "nn_encoder.hpp"

using namespace shogi;

torch::Tensor board_to_tensor(const shogi::Board &board)
{
    torch::Tensor tensor = torch::zeros({1, 29, 9, 9}, torch::kFloat32);

    for (int ch = 0; ch < 28; ch++)
    {
        const Bitboard81 &bb = board.pieceBB[ch];
        for (int sq : bb.squares())
        {
            int x = sq % 9;
            int y = sq / 9;
            tensor[0][ch][y][x] = 1.0f;
        }
    }

    float ply_norm = board.moveNumber / 200.0f;
    if (ply_norm > 1.0f)
        ply_norm = 1.0f;
    tensor[0][28].fill_(ply_norm);

    return tensor;
}
