# Developer Memo

※ このファイルは開発者用の作業メモです  
※ 内容は整理されていないことがあります

---

## ディレクトリ構成

```text
NukoCat/
│
├── cpp/
│   ├── core/         # C++将棋ロジック
│   ├── nn/           # C++ニュースネット定義
│   └── main.cpp      # MCTS探索
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

## 共通仕様

### 将棋盤の扱い

```text
        ┌───┬───┬───┬───┬───┬───┬───┬───┐
rank=8  72  73  74  75  76  77  78  79  80
        ├───┼───┼───┼───┼───┼───┼───┼───┤
        63  64  65  66  67  68  69  70  71
        ├───┼───┼───┼───┼───┼───┼───┼───┤
        54  55  56  57  58  59  60  61  62
        ├───┼───┼───┼───┼───┼───┼───┼───┤
        45  46  47  48  49  50  51  52  53
        ├───┼───┼───┼───┼───┼───┼───┼───┤
        36  37  38  39  40  41  42  43  44
        ├───┼───┼───┼───┼───┼───┼───┼───┤
        27  28  29  30  31  32  33  34  35
        ├───┼───┼───┼───┼───┼───┼───┼───┤
        18  19  20  21  22  23  24  25  26
        ├───┼───┼───┼───┼───┼───┼───┼───┤
        09  10  11  12  13  14  15  16  17
        ├───┼───┼───┼───┼───┼───┼───┼───┤
rank=0  00  01  02  03  04  05  06  07  08
        └───┴───┴───┴───┴───┴───┴───┴───┘
      file=0                          file=8

```

- fileは左から右に 0 → 8
- rankは下から上に 0 → 8
- indexは

```ini
index = 9 * rank + file
```

この定義により：

- 左下 = 0
- 右上 = 80

### コンパイル警告

```bash
/home/ryosuke/NukoCat/cpp/core/Board.cpp: In member function ‘std::vector<Move> shogi::Board::generateMoves(Player) const’:
/home/ryosuke/NukoCat/cpp/core/Board.cpp:421:94: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  421 |                 moves.push_back(Move{fromSq, toSq, PieceType::Pawn, true, PieceType::None});
      |                                                                                          ^

/home/ryosuke/NukoCat/cpp/core/Board.cpp:424:95: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  424 |                moves.push_back(Move{fromSq, toSq, PieceType::Pawn, false, PieceType::None});
      |                                                                                          ^

/home/ryosuke/NukoCat/cpp/core/Board.cpp:425:94: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  425 |                 moves.push_back(Move{fromSq, toSq, PieceType::Pawn, true, PieceType::None});
      |                                                                                          ^

/home/ryosuke/NukoCat/cpp/core/Board.cpp:428:95: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  428 |                moves.push_back(Move{fromSq, toSq, PieceType::Pawn, false, PieceType::None});
      |                                                                                          ^

/home/ryosuke/NukoCat/cpp/core/Board.cpp:445:100: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  445 |               moves.push_back(Move{fromSq, toSq, PieceType::Knight, true, PieceType::None});
      |                                                                                          ^

/home/ryosuke/NukoCat/cpp/core/Board.cpp:448:101: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  448 |              moves.push_back(Move{fromSq, toSq, PieceType::Knight, false, PieceType::None});
      |                                                                                          ^

/home/ryosuke/NukoCat/cpp/core/Board.cpp:449:100: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  449 |               moves.push_back(Move{fromSq, toSq, PieceType::Knight, true, PieceType::None});
      |                                                                                          ^

/home/ryosuke/NukoCat/cpp/core/Board.cpp:452:101: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  452 |              moves.push_back(Move{fromSq, toSq, PieceType::Knight, false, PieceType::None});
      |                                                                                          ^

/home/ryosuke/NukoCat/cpp/core/Board.cpp:484:93: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  484 |                          moves.push_back(Move{fromSq, toSq, po.pt, false, PieceType::None});
      |                                                                                          ^

/home/ryosuke/NukoCat/cpp/core/Board.cpp:485:92: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  485 |                           moves.push_back(Move{fromSq, toSq, po.pt, true, PieceType::None});
      |                                                                                          ^

/home/ryosuke/NukoCat/cpp/core/Board.cpp:488:93: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  488 |                          moves.push_back(Move{fromSq, toSq, po.pt, false, PieceType::None});
      |                                                                                          ^

/home/ryosuke/NukoCat/cpp/core/Board.cpp: In lambda function:
/home/ryosuke/NukoCat/cpp/core/Board.cpp:520:87: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  520 |                         moves.push_back(Move{fromSq, nextSq, pt, true, PieceType::None});
      |                                                                                       ^
/home/ryosuke/NukoCat/cpp/core/Board.cpp:523:88: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  523 |                         moves.push_back(Move{fromSq, nextSq, pt, false, PieceType::None});
      |                                                                                        ^
/home/ryosuke/NukoCat/cpp/core/Board.cpp:524:87: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  524 |                         moves.push_back(Move{fromSq, nextSq, pt, true, PieceType::None});
      |                                                                                       ^
/home/ryosuke/NukoCat/cpp/core/Board.cpp:527:88: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  527 |                         moves.push_back(Move{fromSq, nextSq, pt, false, PieceType::None});
      |                                                                                        ^
/home/ryosuke/NukoCat/cpp/core/Board.cpp: In member function ‘std::vector<Move> shogi::Board::generateMoves(Player) const’:
/home/ryosuke/NukoCat/cpp/core/Board.cpp:570:92: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  570 |               moves.push_back(Move{fromSq, toSq, PieceType::Horse, false, PieceType::None});
      |                                                                                          ^

/home/ryosuke/NukoCat/cpp/core/Board.cpp:598:93: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  598 |              moves.push_back(Move{fromSq, toSq, PieceType::Dragon, false, PieceType::None});
      |                                                                                          ^

/home/ryosuke/NukoCat/cpp/core/Board.cpp:633:72: warning: missing initializer for member ‘Move::gotPiece’ [-Wmissing-field-initializers]
  633 |                 moves.push_back(Move{-1, sq, PieceType::None, false, pt});
      |                                                                        ^
/home/ryosuke/NukoCat/cpp/core/Board.cpp:380:20: warning: variable ‘occ’ set but not used [-Wunused-but-set-variable]
  380 |         Bitboard81 occ = occupied();
      |                    ^~~
[ 64%] Building CXX object CMakeFiles/nukocat_cpp.dir/home/ryosuke/NukoCat/cpp/mcts/policy.cpp.o
[ 71%] Building CXX object CMakeFiles/nukocat_cpp.dir/home/ryosuke/NukoCat/cpp/mcts/state.cpp.o
[ 78%] Building CXX object CMakeFiles/nukocat_cpp.dir/home/ryosuke/NukoCat/cpp/nn/nn_encoder.cpp.o
[ 85%] Building CXX object CMakeFiles/nukocat_cpp.dir/home/ryosuke/NukoCat/cpp/nn/nn_model.cpp.o
[ 92%] Building CXX object CMakeFiles/nukocat_cpp.dir/main.cpp.o
[100%] Linking CXX shared module nukocat_cpp.cpython-310-x86_64-linux-gnu.so
lto-wrapper: warning: using serial compilation of 4 LTRANS jobs
[100%] Built target nukocat_cpp
```
