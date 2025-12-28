import torch


def load_move_dicts(path):
    import pickle

    with open(path, "rb") as f:
        obj = pickle.load(f)

    print("[DEBUG] move2idx.pkl type:", type(obj))
    print("[DEBUG] content keys:", obj.keys() if isinstance(obj, dict) else obj)

    return obj["move2idx"], obj["idx2move"]


# -----------------------------------
# 合法手マスク
# -----------------------------------
def legal_moves_mask(board, move2idx):
    mask = torch.zeros(len(move2idx), dtype=torch.bool)
    for move in board.legal_moves:
        usi = move.usi()
        idx = move2idx.get(usi)
        if idx is not None:
            mask[idx] = True
    return mask
