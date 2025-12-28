"""
NukoCat の将棋用 Policy-Value Network 定義をまとめたパッケージ。

このパッケージは学習・評価コード内部からのみ使用されることを想定しており、
外部ライブラリとしての公開 API は定義しない。

構成:
- ResidualBlock:
    ResNet 形式の残差ブロック。
- ShogiPolicyValueNet:
    将棋盤 (9x9) 入力を受け取り、
    方策 (policy) と価値 (value) を同時に出力するネットワーク。

注意:
- 推論専用の軽量モデルや TorchScript 用モデルは別途定義する想定。
- このパッケージ内のクラス構成は学習コードに強く依存するため、
  直接 import しての再利用は推奨しない。
"""
