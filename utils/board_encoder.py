import torch
import numpy as np
import shogi


def board_to_tensor(board: shogi.Board, ply: int):
    """
    出力 shape: (29, 9, 9)
    0-27 : 盤面（先後×14駒種）
    28   : ply（手数）チャンネル
    """
    tensor = np.zeros((29, 9, 9), dtype=np.float32)

    # --- 盤面 28ch ---
    for sq in range(81):
        piece = board.piece_at(sq)
        if piece is None:
            continue
        x = sq % 9
        y = sq // 9
        piece_type = piece.piece_type - 1
        color = 0 if piece.color == shogi.BLACK else 1
        ch = piece_type + 14 * color
        tensor[ch, y, x] = 1.0

    # --- ply 1ch（正規化） ---
    ply_norm = min(ply / 200.0, 1.0)
    tensor[28, :, :] = ply_norm

    return torch.from_numpy(tensor)
