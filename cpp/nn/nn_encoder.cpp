#include "nn/nn_encoder.hpp"
#include "config/constants.hpp"

using namespace shogi;

torch::Tensor board_to_tensor(const Board &board)
{
    // (1, 43, 9, 9)
    torch::Tensor tensor =
        torch::zeros({1, config::INPUT_CHANNELS, 9, 9}, torch::kFloat32);

    // =========================
    // 盤面 28ch（先後 × 14駒種）
    // =========================
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

    // =========================
    // 持ち駒 14ch
    // 28–34 : 先手
    // 35–41 : 後手
    // =========================
    static const PieceType HAND_PIECES[7] = {
        PieceType::Pawn,
        PieceType::Lance,
        PieceType::Knight,
        PieceType::Silver,
        PieceType::Gold,
        PieceType::Bishop,
        PieceType::Rook};

    // --- 先手 ---
    for (int i = 0; i < 7; i++)
    {
        PieceType pt = HAND_PIECES[i];
        auto it = board.senteHand.find(pt);
        if (it != board.senteHand.end() && it->second > 0)
        {
            tensor[0][28 + i].fill_(static_cast<float>(it->second));
        }
    }

    // --- 後手 ---
    for (int i = 0; i < 7; i++)
    {
        PieceType pt = HAND_PIECES[i];
        auto it = board.goteHand.find(pt);
        if (it != board.goteHand.end() && it->second > 0)
        {
            tensor[0][35 + i].fill_(static_cast<float>(it->second));
        }
    }

    // =========================
    // ply 1ch（ch = 42）
    // =========================
    float ply_norm = board.moveNumber / 200.0f;
    if (ply_norm > 1.0f)
        ply_norm = 1.0f;

    tensor[0][42].fill_(ply_norm);

    return tensor;
}
