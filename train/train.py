import os
import argparse
import pickle

import torch
import torch.nn.functional as F
from torch.utils.data import DataLoader

import shogi
import shogi.CSA

from train.train_dataset import ShogiDataset
from nn.model.policy_value_net import ShogiPolicyValueNet

from nn.encoder.board_encoder import INPUT_CHANNELS


def export_move_dicts_txt(save_dir, idx2move):
    """
    C++ 用に index -> usi の対応表を txt で出力
    """
    txt_path = os.path.join(save_dir, "move_dicts.txt")
    with open(txt_path, "w", encoding="utf-8") as f:
        for idx in range(len(idx2move)):
            f.write(f"{idx} {idx2move[idx]}\n")

    print(f"[INFO] move_dicts.txt exported: {txt_path}")


def train_loop(args):
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"[INFO] device = {device}")

    # =======================
    # CSA 読み込み
    # =======================
    csa_folder = os.path.join("static", "csa", args.username)
    csa_files = [f for f in os.listdir(csa_folder) if f.endswith(".csa")]

    if not csa_files:
        raise RuntimeError("CSA ファイルが見つかりません")

    # =======================
    # move2idx / idx2move
    # =======================
    all_moves = set()
    for f in csa_files:
        try:
            games = shogi.CSA.Parser.parse_file(os.path.join(csa_folder, f))
        except Exception:
            continue

        for g in games:
            for m in g.get("moves", []):
                all_moves.add(m)

    move2idx = {m: i for i, m in enumerate(sorted(all_moves))}
    idx2move = {i: m for m, i in move2idx.items()}
    num_moves = len(move2idx)

    print(f"[INFO] num_moves = {num_moves}")

    # =======================
    # Model
    # =======================
    model = ShogiPolicyValueNet(num_moves=num_moves).to(device)
    optimizer = torch.optim.Adam(model.parameters(), lr=args.lr)

    # =======================
    # Dataset / DataLoader
    # =======================
    dataset = ShogiDataset(
        csa_files=csa_files,
        csa_folder=csa_folder,
        move2idx=move2idx,
        myname=args.username,
        device=device,
    )

    if len(dataset) == 0:
        raise RuntimeError("学習サンプルが 0 件です")

    dataloader = DataLoader(
        dataset,
        batch_size=args.batch_size,
        shuffle=True,
        drop_last=True,
    )

    # =======================
    # Training loop
    # =======================
    for epoch in range(args.epochs):
        model.train()

        policy_loss_sum = 0.0
        value_loss_sum = 0.0

        for step, (x, mask, policy_y, value_y) in enumerate(dataloader):
            x = x.to(device)
            mask = mask.to(device)
            policy_y = policy_y.to(device)
            value_y = value_y.to(device).float()

            policy_logits, value_pred = model(x)

            # 合法手マスク
            policy_logits = policy_logits.masked_fill(~mask, -1e9)

            # policy loss
            log_probs = F.log_softmax(policy_logits, dim=1)
            policy_loss = F.nll_loss(log_probs, policy_y)

            # value loss（棋風局面分布）
            value_loss = F.mse_loss(value_pred.squeeze(), value_y)

            # 合成 loss
            loss = policy_loss + args.value_weight * value_loss

            optimizer.zero_grad()
            loss.backward()
            optimizer.step()

            policy_loss_sum += policy_loss.item()
            value_loss_sum += value_loss.item()

            if step % 50 == 0:
                print(
                    f"[Epoch {epoch+1}] step={step} "
                    f"policy={policy_loss.item():.4f} "
                    f"value_mean={value_pred.mean().item():.4f} "
                    f"value_std={value_pred.std().item():.4f}"
                )

        print(
            f"[Epoch {epoch+1} DONE] "
            f"Policy={policy_loss_sum/len(dataloader):.4f} "
            f"Value={value_loss_sum/len(dataloader):.4f}"
        )

    # =======================
    # Save (PyTorch)
    # =======================
    save_dir = os.path.join("trained_models", args.username)
    os.makedirs(save_dir, exist_ok=True)

    model_path = os.path.join(save_dir, "policy_value.pth")
    torch.save(model.state_dict(), model_path)

    with open(os.path.join(save_dir, "move_dicts.pkl"), "wb") as f:
        pickle.dump(
            {"move2idx": move2idx, "idx2move": idx2move},
            f,
        )

    print("[INFO] PyTorch model + move_dicts.pkl saved")

    # =======================
    # TorchScript export
    # =======================
    print("[INFO] Exporting TorchScript model...")

    model_cpu = ShogiPolicyValueNet(num_moves=num_moves)
    model_cpu.load_state_dict(torch.load(model_path, map_location="cpu"))
    model_cpu.eval()

    example_input = torch.randn(1, INPUT_CHANNELS, 9, 9)
    scripted_model = torch.jit.trace(model_cpu, example_input)

    ts_path = os.path.join(save_dir, "policy_value.ts")
    scripted_model.save(ts_path)

    print(f"[INFO] TorchScript saved: {ts_path}")

    # =======================
    # move_dicts.txt export
    # =======================
    export_move_dicts_txt(save_dir, idx2move)

    print("[INFO] Training + Export finished successfully")


# =======================
# Entry point
# =======================
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--username", required=True)
    parser.add_argument("--epochs", type=int, default=10)
    parser.add_argument("--batch-size", type=int, default=32)
    parser.add_argument("--lr", type=float, default=1e-3)

    # ★重要：policy を壊さない現実的な重み
    parser.add_argument("--value-weight", type=float, default=0.1)

    args = parser.parse_args()
    train_loop(args)
