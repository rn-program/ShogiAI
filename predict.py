import os
import sys

import torch
import pickle

from model.policy_value_net import ShogiPolicyValueNet
from utils.board_encoder import board_to_tensor
from utils.move_encoder import legal_moves_mask

import shogi

# ======================
# パス設定
# ======================
PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
CPP_PYBIND_BUILD = os.path.join(PROJECT_ROOT, "cpp", "pybind", "build")

sys.path.insert(0, CPP_PYBIND_BUILD)

import nukocat_cpp as nc


def predict_best_move_policy_only(
    sfen: str,
    model_path: str,
    move_dict_path: str,
    device: str = "cpu",
    topk: int = 5,
):
    """
    MCTSなし・policy networkのみで指し手を予測
    上位 topk 手と確率を返す
    """

    # --------------------
    # move_dicts 読み込み
    # --------------------
    with open(move_dict_path, "rb") as f:
        move_dicts = pickle.load(f)

    move2idx = move_dicts["move2idx"]
    idx2move = move_dicts["idx2move"]

    # --------------------
    # モデル読み込み
    # --------------------
    model = ShogiPolicyValueNet(num_moves=len(move2idx))
    model.load_state_dict(torch.load(model_path, map_location=device))
    model.to(device)
    model.eval()

    # --------------------
    # 局面準備
    # --------------------
    board = shogi.Board(sfen)
    ply = board.move_number

    x = board_to_tensor(board, ply)

    if not isinstance(x, torch.Tensor):
        x = torch.from_numpy(x)

    x = x.unsqueeze(0).to(device)

    legal_mask = legal_moves_mask(board, move2idx)

    if not isinstance(legal_mask, torch.Tensor):
        legal_mask = torch.from_numpy(legal_mask)

    legal_mask = legal_mask.unsqueeze(0).to(device)

    # --------------------
    # 推論
    # --------------------
    with torch.no_grad():
        policy_logits, _ = model(x)

        # 非合法手を強制的に除外
        policy_logits = policy_logits.masked_fill(legal_mask == 0, -1e9)

        probs = torch.softmax(policy_logits, dim=1)[0].cpu().numpy()

    # --------------------
    # 上位手抽出
    # --------------------
    ranked = sorted(
        [(idx2move[i], float(probs[i])) for i in range(len(probs)) if probs[i] > 0],
        key=lambda x: x[1],
        reverse=True,
    )

    best_move = ranked[0][0]
    top_moves = ranked[:topk]

    return best_move, top_moves


def predict_best_move_mcts(
    sfen: str,
    username: str,
    simulations: int = 1000,
):
    """
    C++ (pybind11) 側の NN + MCTS を使って 1 手予測する
    main.cpp の search() に完全対応
    """

    # C++ が期待する model_dir を組み立てる
    model_dir = os.path.join(
        PROJECT_ROOT,
        "trained_models",
        username,
    )

    move_usi = nc.search(
        model_dir=model_dir,
        sfen=sfen,
        simulations=simulations,
    )

    if not move_usi:
        return None

    return shogi.Move.from_usi(move_usi)


# ======================
# CLI
# ======================
if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--username", required=True)
    parser.add_argument("--sfen", type=str, default=None)
    parser.add_argument("--simulations", type=int, default=1000)
    parser.add_argument("--topk", type=int, default=5)
    args = parser.parse_args()

    model_path = f"trained_models/{args.username}/policy_value.pth"
    move_dict_path = f"trained_models/{args.username}/move_dicts.pkl"

    # 初期局面 or 指定局面
    if args.sfen is None:
        sfen = shogi.Board().sfen()
    else:
        sfen = args.sfen

    best, top_moves = predict_best_move_policy_only(
        sfen=sfen,
        model_path=model_path,
        move_dict_path=move_dict_path,
        topk=args.topk,
    )

    print("Predicted move (policy only):", best)
    print("Top moves:")
    for usi, p in top_moves:
        print(f"  {usi} : {p:.4f}")

    move = predict_best_move_mcts(
        sfen=sfen,
        username=args.username,
        simulations=args.simulations,
    )

    print("Predicted move (MCTS):", move.usi() if move else "None")
