import os
import shogi
import shogi.CSA
import torch
from torch.utils.data import Dataset

from nn.encoder.board_encoder import board_to_tensor
from nn.encoder.move_encoder import legal_moves_mask


class ShogiDataset(Dataset):
    """
    Policy-only Dataset（棋譜模倣）
    - 自分の手番のみを学習
    - value は一切使用しない
    """

    def __init__(
        self,
        csa_files,
        csa_folder,
        move2idx,
        myname,
        device="cpu",
    ):
        self.samples = []
        self.move2idx = move2idx
        self.device = device

        total_games = 0
        total_samples = 0

        for csa_file in csa_files:
            full = os.path.join(csa_folder, csa_file)

            try:
                games = shogi.CSA.Parser.parse_file(full)
            except Exception as e:
                print(f"[SKIP] parse failed: {csa_file} ({e})")
                continue

            # ヘッダ読み込み
            try:
                with open(full, "r", encoding="utf-8", errors="ignore") as f:
                    lines = f.readlines()
            except Exception:
                lines = []

            black_name = None
            white_name = None
            for ln in lines:
                if ln.startswith("N+"):
                    black_name = ln[2:].strip().split()[0]
                elif ln.startswith("N-"):
                    white_name = ln[2:].strip().split()[0]

            for game in games:
                total_games += 1

                # 学習側決定
                if black_name == myname:
                    learn_side = shogi.BLACK
                elif white_name == myname:
                    learn_side = shogi.WHITE
                else:
                    continue

                board = shogi.Board()

                positions = []
                moves = []

                for move in game.get("moves", []):
                    if board.turn == learn_side:
                        positions.append(board.sfen())
                        moves.append(move)
                    try:
                        board.push_usi(move)
                    except Exception:
                        break

                for sfen, move in reversed(list(zip(positions, moves))):
                    board = shogi.Board(sfen)
                    ply = board.move_number

                    x = board_to_tensor(board, ply)
                    if not isinstance(x, torch.Tensor):
                        x = torch.from_numpy(x)
                    x = x.to(self.device)

                    assert x.shape[0] == 43, x.shape

                    mask = legal_moves_mask(board, move2idx)
                    if not isinstance(mask, torch.Tensor):
                        mask = torch.from_numpy(mask)

                    policy_idx = move2idx.get(move)
                    if policy_idx is None:
                        continue

                    self.samples.append((x, mask, policy_idx))
                    total_samples += 1

        print(f"  games parsed : {total_games}")
        print(f"  samples     : {total_samples}")

    def __len__(self):
        return len(self.samples)

    def __getitem__(self, idx):
        return self.samples[idx]
