import json
import os

import time

import shogi
import predict


def self_play_policy_only(username: str, max_moves: int = 500):
    board = shogi.Board()
    moves = []

    timestamp = time.strftime("%Y%m%d_%H%M%S")

    base_dir = f"self_play_logs/{username}/{timestamp}"
    os.makedirs(base_dir, exist_ok=True)

    settings = {
        "mode": "policy_only",
        "username": username,
        "max_moves": max_moves,
        "timestamp": timestamp,
    }

    played_moves = []
    policy_outputs = []

    ply = 1

    while not board.is_game_over():
        _, top_moves = predict.predict_best_move_policy_only(
            sfen=board.sfen(),
            model_path=f"trained_models/{username}/policy_value.pth",
            move_dict_path=f"trained_models/{username}/move_dicts.pkl",
            topk=5,
        )

        # ----------------------
        # policy 分布
        # ----------------------
        policy_dict = {usi: float(p) for usi, p in top_moves}

        policy_outputs.append(
            {
                "ply": ply,
                "sfen": board.sfen(),
                "policy": policy_dict,
            }
        )

        # ----------------------
        # 実際に指す手 + 確率
        # ----------------------
        move_usi, move_prob = top_moves[0]
        move = shogi.Move.from_usi(move_usi)

        if move is None:
            break

        board.push(move)
        moves.append(move.usi())

        played_moves.append(
            {
                "ply": ply,
                "move": move_usi,
                "prob": float(move_prob),
            }
        )

        print(f"Move played: {move_usi} (p={move_prob:.4f})")

        ply += 1

        if len(moves) >= max_moves:
            break

    # ======================
    # JSON 保存
    # ======================
    played_path = os.path.join(base_dir, f"played_moves_{timestamp}.json")
    policy_path = os.path.join(base_dir, f"policy_outputs_{timestamp}.json")

    played_json = {
        "settings": settings,
        "moves": played_moves,
    }

    policy_json = {
        "settings": settings,
        "policies": policy_outputs,
    }
    with open(played_path, "w", encoding="utf-8") as f:
        json.dump(played_json, f, ensure_ascii=False, indent=2)

    with open(policy_path, "w", encoding="utf-8") as f:
        json.dump(policy_json, f, ensure_ascii=False, indent=2)

    print("[INFO] saved json logs")
    print(f"  {played_path}")
    print(f"  {policy_path}")

    return moves


def self_play_mcts(username: str, simulations: int = 1000, max_moves: int = 500):
    board = shogi.Board()
    moves = []

    timestamp = time.strftime("%Y%m%d_%H%M%S")
    base_dir = f"self_play_logs/{username}/{timestamp}MCTS"
    os.makedirs(base_dir, exist_ok=True)

    settings = {
        "mode": "mcts",
        "username": username,
        "simulations": simulations,
        "max_moves": max_moves,
        "timestamp": timestamp,
    }

    played_moves = []
    policy_outputs = []

    ply = 1

    while not board.is_game_over():
        # ----------------------
        # policy 分布（観測用）
        # ----------------------
        _, top_moves = predict.predict_best_move_policy_only(
            sfen=board.sfen(),
            model_path=f"trained_models/{username}/policy_value.pth",
            move_dict_path=f"trained_models/{username}/move_dicts.pkl",
            topk=5,
        )

        policy_dict = {usi: float(p) for usi, p in top_moves}
        policy_outputs.append(
            {
                "ply": ply,
                "sfen": board.sfen(),
                "policy": policy_dict,
            }
        )

        # ----------------------
        # MCTS による着手
        # ----------------------
        move = predict.predict_best_move_mcts(
            sfen=board.sfen(),
            username=username,
            simulations=simulations,
        )

        if move is None:
            break

        board.push(move)
        moves.append(move.usi())

        played_moves.append(
            {
                "ply": ply,
                "move": move.usi(),
            }
        )

        print(f"Move played (MCTS): {move.usi()}")

        ply += 1
        if len(moves) >= max_moves:
            break

    # ======================
    # JSON 保存
    # ======================
    played_path = os.path.join(base_dir, f"played_moves_{timestamp}.json")
    policy_path = os.path.join(base_dir, f"policy_outputs_{timestamp}.json")

    played_json = {
        "settings": settings,
        "moves": played_moves,
    }

    policy_json = {
        "settings": settings,
        "policies": policy_outputs,
    }
    with open(played_path, "w", encoding="utf-8") as f:
        json.dump(played_json, f, ensure_ascii=False, indent=2)

    with open(policy_path, "w", encoding="utf-8") as f:
        json.dump(policy_json, f, ensure_ascii=False, indent=2)

    print("[INFO] saved json logs (MCTS)")
    print(f"  {played_path}")
    print(f"  {policy_path}")

    return moves


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--username", required=True)
    parser.add_argument("--simulations", type=int, default=1000)
    parser.add_argument("--max-moves", type=int, default=500)
    parser.add_argument("--use-mcts", action="store_true", help="use MCTS")
    args = parser.parse_args()

    if args.use_mcts:
        print("Starting self-play with MCTS...")
        moves = self_play_mcts(
            username=args.username,
            simulations=args.simulations,
            max_moves=args.max_moves,
        )

    else:
        print("Starting self-play with policy only...")
        moves = self_play_policy_only(
            username=args.username,
            max_moves=args.max_moves,
        )

    print("Self-play moves:", moves)
