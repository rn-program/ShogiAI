import torch
import numpy as np
import shogi

INPUT_CHANNELS = 43

def board_to_tensor(board: shogi.Board, ply: int):
    """
    出力 shape: (43, 9, 9)

    0-27  : 盤面（先後 × 14駒種）
    28-41 : 持ち駒（先後 × 7駒種）
    42    : ply（手数）チャンネル
    """
    tensor = np.zeros((INPUT_CHANNELS, 9, 9), dtype=np.float32)

    # =========================
    # 盤面（28ch）
    # =========================
    for sq in range(81):
        piece = board.piece_at(sq)
        if piece is None:
            continue

        x = sq % 9
        y = sq // 9

        piece_type = piece.piece_type - 1  # 0-13
        color = 0 if piece.color == shogi.BLACK else 1
        ch = piece_type + 14 * color

        tensor[ch, y, x] = 1.0

    # =========================
    # 持ち駒（14ch）
    # =========================
    # 持ち駒に存在する駒種（成り駒なし）
    hand_piece_types = [
        shogi.PAWN,
        shogi.LANCE,
        shogi.KNIGHT,
        shogi.SILVER,
        shogi.GOLD,
        shogi.BISHOP,
        shogi.ROOK,
    ]

    for color in [shogi.BLACK, shogi.WHITE]:
        color_offset = 0 if color == shogi.BLACK else 7
        hands = board.pieces_in_hand[color]

        for i, ptype in enumerate(hand_piece_types):
            count = hands.get(ptype, 0)
            ch = 28 + color_offset + i

            # 枚数をそのまま敷き詰める
            tensor[ch, :, :] = float(count)

    # =========================
    # ply（1ch）
    # =========================
    ply_norm = min(ply / 200.0, 1.0)
    tensor[42, :, :] = ply_norm

    return torch.from_numpy(tensor)
