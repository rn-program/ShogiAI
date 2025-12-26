# NukoCat

**NukoCat** は、特定の将棋プレーヤーの棋風を模倣した将棋 AI を  
比較的容易に作成・検証することを目的としたプロジェクトです。

学習データとして棋譜を用意することで、

- 特定プレーヤーの指し回し傾向を反映した AI
- プレーヤーごとの棋風差異を比較可能な AI

を、簡単な手順で構築できます。

---

## 特徴

- ♟️ **棋風特化型 AI**：特定プレーヤーの棋譜のみを用いた模倣学習
- 🧠 **ポリシー・バリューネットワーク中心設計**：次の一手の選択傾向を重視

---

## 想定使用ケース

- 特定の将棋プレーヤーの棋風を再現したい場合
- アマ強豪等の棋譜を用いた疑似対局・練習用途

---

## 再現性・強さについて

- 本 AI は棋風模倣を目的としており、最善手の選択や強さを保証するものではありません
- 棋譜数が少ない場合、学習結果が不安定になることがあります

---

## 動作環境

- Python 3.10（Python3.10 をベースに開発を進行中）
- C++17 以上
- OS: Linux / Windows（WSL 推奨）
- GPU: 非対応（学習に時間を要すため、将来的に対応させる予定）

### 必要ライブラリのインストール

---

python ライブラリ：

```bash
pip install -r requirements.txt \
  --index-url https://download.pytorch.org/whl/cpu
```

※PyTorch のインストールは現状、CPUバージョンのみ対応しています。

---

## ディレクトリ構成

```text
NukoCat/
│
├── cpp/
│   ├── core/         # C++将棋ロジック
│   ├── mcts/         # MCTS探索
│   └── nn/           # C++ニュースネット定義
│
├── model/            # ネットワーク定義
│   └── policy_value_net.py
│
├── static/           # 棋譜データ（CSA）・ GUI関連
│
├── train/            # 学習関連コード
│   ├── train.py
│   └── train_dataset.py
│
├── utils/            # 補助関数
│
├── app.py            # 対局用GUI
│
└── predict.py        # 指し手予測コード
```

※ 実際の構成は変更される可能性があります。

---

## 基本的な使い方（概要）

### 1. 棋譜データを用意

CSA 形式の棋譜をプレーヤーごとに用意し、static/csa に配置してください。

getCSA.py を使用して、将棋ウォーズのアカウントを指定することで
直近 1 か月分の棋譜を自動取得することもできます

```bash
python getCSA.py --username <プレーヤー名>
```

実行後、static/csa/<プレーヤー名>に CSA ファイルが生成されます。

---

### 2. 学習

```bash
python -m train.train \
--username <プレーヤー名> \
--epochs <学習回数> \
--batch-size <一学習での処理局面数> \
--lr <学習率> \
--value-weight <valueの重み>
```

実行後、以下のディレクトリが自動生成されます。

```text
trained_models/<プレーヤー名>/
```

生成される主なファイル：

- move_dicts.txt：モデル出力インデックスと将棋の指し手（USI）の対応表（C++ 用）
- policy_value.ts：学習済み Policy / Value ネットワーク（TorchScript）

---

## コマンドライン引数の説明

---

### --username (必須)

学習対象となるプレーヤー名を指定します。

```text
static/csa_models/<ユーザー名>/
```

に配置された CSA 棋譜ファイルを学習データとして使用します。

学習結果は

```text
trained_models/<ユーザー名>/
```

に保存されます

---

### --epochs (default=10)

学習データ全体を何周学習するかを指定します。

値を大きくすると学習は進みますが、過学習となる可能性があります。

---

### batch-size (default=32)

1 回の学習ステップで同時に処理する局面数を指定します。

学習の安定性に影響します。

---

### lr (default=1e-3)

学習率（Learning Rate）を指定します。

モデルの重み更新量を制御します。

---

### value-weight (default=1.0)

Value ネットワーク（局面の有利不利予測）の損失にかける重みを指定します。

Policy と Value のどちらを重視するかの調整に使用します。

---

### 3. C++探索コードのビルド

詳細は~/NukoCat/cpp/README.md を参考にして下さい。

---

### 4. 推論・対局

---

#### 自分 vs AI

```bash
python app.py -- --username <プレーヤー名> --ai_turn <AIの手番> --simulations <シュミレーション回数>
```

AI の手番は 0 で先手、1 で後手を表します。

実行すると、自動で対局用将棋盤 GUI が開きます。

---

#### AI 自己対局

```bash
python self_play.py --username <プレーヤー名> --simulations <シュミレーション回数> --max-moves <手数の上限> --use-mcts <MCTS探索を用いるかどうか>
```

---

対局後には以下のディレクトリ：

```bash
self_play_logs/<ユーザー名>
```

が自動で生成され、ディレクトリ内に以下のファイルが生成されます。

- played_moves.json：棋譜(.json)ファイル
- policy_outputs.json：ポリシー推論のTop5の手と予測確率の対応表(.json)

生成されたplayed_moves.jsonはkifu_viewer.pyで棋譜を確認できます。

```bash
python ./kifu_viewer.py -- --kifu <棋譜ファイルパス>
```

※ kifu_viewer.pyは予測モデルが正常に動作しているかの確認用GUIであるため、デザインはかなり簡素なものになっています。

---

## 開発の目的

既存の強さ重視の将棋 AI とは異なり、NukoCat は

> **「誰の将棋か」**

に注目した AI の構築を目的としています。

強さだけでなく、

- 指し手の好み
- 駒組みの傾向
- 中終盤の志向

などを再現・観察するための研究的・実験的な基盤として設計されています。

---

## 使用技術

- Python
- PyTorch
- python-shogi
- CppShogi

---

## バージョン情報

- Version: 0.1.0
- Status: 開発中(experimental)

---

## 今後の改良予定

- GUI のデザイン改良
- 実際の最善手と棋風とのバランスを最適化
- 棋譜再現制度の向上
- CppShogi の完全化 (打ち歩詰め・連続王手の千日手等)

---

## 修正予定のバグ

- 特になし

---

## ライセンス

本プロジェクトは MIT License のもとで公開されています。  
詳細は `LICENSE` ファイルを参照してください。

---

## 注意事項

- 本プロジェクトは将棋プレーヤーの棋風に関する研究・学習目的で開発され、実在の棋士・プレーヤーの評価を目的とするものではありません
- 棋譜の著作権は各権利者に帰属します。

---

## 開発者情報

- 開発者: NukoNeko693
- 将棋ウォーズアカウント: Orangeapple10 (3 切れ四段)

---
