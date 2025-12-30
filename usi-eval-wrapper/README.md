# usi-eval-wrapper

USI対応将棋エンジンを外部プロセスとして起動し、
`fork + exec + pipe` により評価値（score cp）を取得する
最小構成の C++ ラッパー実装です。

本リポジトリは **将棋エンジン本体を含みません**。

---

## 特徴

- USIプロトコル対応
- fork / exec による完全なプロセス分離
- 標準入力・出力を pipe で接続
- depth 1 の評価値（cp）取得に特化
- Linux 環境向け（POSIX）

---

## できること

- 外部USIエンジンを起動
- `position sfen` を送信
- `go depth 1`
- `info score cp xxx` を取得

---

## できないこと

- エンジンの改変
- 評価関数（nn.bin 等）の配布
- 探索制御や長時間探索

---

## 依存関係

- Linux
- C++17
- POSIX API（fork / exec / pipe）

---

## 使い方（例）

```cpp
YaneuraOuValue eval("/path/to/usi_engine", true);
int cp = eval.evaluate(sfen);
```

---

## 注意

本リポジトリは USIエンジンを外部プロセスとして起動するだけであり、やねうら王本体や評価関数は含まれていません。

各USIエンジンのライセンスはそれぞれに従ってください

## 動作確認

- Ubuntu 22.04
- g++ 11
- YaneuraOu (USI engine mode)

## ライセンス

- MIT License
