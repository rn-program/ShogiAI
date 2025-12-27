# NukoCat MCTS Engine 構成解説

このディレクトリは、**AlphaZero 系の学習・自己対局に対応した MCTS（Monte Carlo Tree Search）エンジン**を
責務分離を重視した設計で実装しています。

各ファイルは「1 ファイル = 1 役割」を厳密に守っており、
将棋ルール・ニューラルネット・探索ロジックが明確に分離されています。

---

## 全体構成図

```css
[ State ] ── ゲーム状態（immutable）
    │
    ├─ is_terminal / terminal_value
    │
[ Policy ] ── 合法手 + policy 確率
    │
[ Evaluator ] ── NN 推論（policy + value）
    │
[ Node / Edge ] ── 探索木の数学的状態
    │
[ MCTS ] ── 探索アルゴリズム本体

```

---

## ファイル一覧と役割

### mcts.hpp / mcts.cpp

探索アルゴリズムの中核

- PUCT による Selection
- Expansion / Evaluation / Backup
- Dirichlet noise（学習用）
- temperature 付き行動選択
- 再帰的シミュレーション制御

**「どの手をどれだけ探索するか」**だけを担当し、
将棋のルールや NN の詳細は知らない。

---

### node.hpp / node.cpp

探索木のノード（局面）

1 局面 = 1 Node

保持情報：

- N : 訪問回数
- W : 累積価値
- Q : 平均価値
- P : policy prior
- children : 次の局面への分岐

MCTS の数理状態そのもの。

---

### edge.hpp / edge.cpp

親ノードから子ノードへの「手（エッジ）」

- 指し手（USI）
- policy prior
- visit / value

**「どの手を通ってこの局面に来たか」**を明確に分離するための構造。
AlphaZero 論文に近い設計。

※ Node に直接指し手情報を持たせないため、設計が破綻しにくい。

---

### policy.hpp / policy.cpp

合法手生成 + policy 確率処理

- Board から合法手を列挙
- NN の policy logits を合法手でマスク
- 正規化された確率分布を生成

**「指せる手が何か」**だけを知っている層。
MCTS の探索ロジックや value には関与しない。

---

### evaluator.hpp / evaluator.cpp

ニューラルネット評価器

-NN 推論の実体（Torch / ONNX / pybind など）
-policy logits と value を返す
-NN 実装の詳細を完全に隠蔽

**「この局面がどれくらい良いか」**を返すブラックボックス。

---

### state.hpp / state.cpp

ゲーム状態（immutable）

- const shogi::Board を保持
- apply() は新しい State を返す（破壊しない）
- is_terminal()：終端判定
- terminal_value()：終局時の価値（手番視点）

MCTS が触る**唯一の将棋ルール窓口**。

---

## 設計上の重要な方針

### immutable State

- Board は const
- undo 不要
- 並列探索・学習に安全
- バグが局所化する

---

### const-correct 設計

- 読み取り専用処理はすべて const
- 状態破壊をコンパイル時に防止

---

### 依存関係の整理

```text
MCTS
 ├── Node
 │    └── Edge
 ├── State
 │    └── Board
 ├── Policy
 │    └── Board
 └── Evaluator
      └── Neural Network

```

- 循環依存なし
- 各層は交換可能
- 将棋以外のゲームにも転用可能

---

### 学習用 MCTS として備えている要素

- PUCT
- value の手番反転
- Dirichlet noise（root のみ）
- temperature 付きサンプリング
- visit 数による行動選択
- NN policy / value 分離

**自己対局・強化学習にそのまま使用可能。**
