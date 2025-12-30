# bugs.md

※ このファイルは開発時のバグに関するメモです

## 2025-12-26 (解決済み)

MCTS が NN policy 上位に無い手を高確率で選択する

---

## 発生状況

- NN の policy 出力では明確に一手が突出している  
  （例：初手 `7g7f ≈ 0.999`）
- にもかかわらず、MCTS の探索結果では  
  policy 上位に無い手（例：`6i7h`, `6g6f`）が選択される
- Dirichlet noise を無効化しても再現する

---

## 初期の誤認

以下を疑ったが、いずれも **原因ではなかった**：

- MCTS の PUCT 実装ミス
- temperature サンプリングの仕様
- Dirichlet noise の影響
- masked_policy による 0 確率手の混入
- 辞書に存在しない手の生成

これらはすべて **正常動作**であることを確認済み。

---

## 決定的な症状

### NN が出力している policy（Python 側）

```json
"policy": {
  "7g7f": 0.99968,
  "2g2f": 0.00022,
  "7i6h": 0.00006
}

### MCTS 内部で使用されていた policy（C++ 側）

```powershell
6i7h P=0.359
6g6f P=0.195
2g2f P=0.156
7g7f P=0.012
```

NN の Top1 と、MCTS 内部の最大 P が 一致していない

## 原因（確定）

**NN の policy_logits の index と、
C++ 側の move2idx_ / idx2move_ の対応が一致していなかった。**

具体的には、Python 側（学習時）で使用した move list の

- 並び順
- 総数
- USI 表記
- C++ 側（推論・MCTS）で使用している辞書

これらが 完全に同一ではなかった。

その結果：

- NN が「7g7f に高確率」を出力
- C++ 側では その logit を別の USI に割り当て
- MCTS は渡された P を正しく使っただけ
- 人間から見ると「意味不明な手」を選択しているように見えた

なぜ Python 側で再学習すると直ったのか

- 学習時の move list が現在の C++ 側辞書と一致したため
- policy_logits の各 index が、正しい USI と再び 1 対 1 に対応した
MCTS の挙動が NN policy と一致するようになった

## 再発防止策（必須）

### move list の単一ソース化

- moves.txt 等を 1 つ用意
- Python / C++ の両方が 同じファイルを読み込む
- 自動生成・別管理をしない

### 起動時の整合性チェック

C++ 側

```cpp
assert(policy_logits.size() == idx2move_.size());
```

Python 側

```python
assert model.policy_head.out_features == len(move_list)
```

### デバッグ用可視化の常設

- NN policy 上位 N 手を USI 付きで出力
- MCTS root の P / N / Q を随時確認可能にする

## 教訓

- MCTS が変な手を指すとき、**最初に疑うべきは探索ロジックではなく「policy の対応関係」**
- AlphaZero 系実装では**辞書不整合が最も致命的かつ発見しづらい**

## 現在の状態

- 原因：解決済み
- MCTS 実装：正常
- NN policy と探索結果：一致
