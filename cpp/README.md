# PyNukoCat (pybind11 ビルド)

C++ で実装された NukoCat 将棋 AI を **Python から呼び出せる pybind11 モジュール** としてビルドする手順です。  
ビルドが成功すると、Python から NN 推論や MCTS 探索を直接利用できます。

---

## ディレクトリ構成例

```text
NukoCat/
├── cpp/
│ ├── core/ # 将棋ロジック (Board, Move, Piece など)
│ ├── mcts/ # MCTS 探索
│ ├── nn/ # NN encoder, board_to_tensor など
│ ├── pybind/ # pybind11 用コード・CMakeLists.txt
│ │
│ └── main.cpp # pybind11 バインディング
│
└── predict.py # python 指し手予測コード
```

---

## 1. CMakeLists.txt

`~/NukoCat/cpp/pybind/CMakeLists.txt` に以下を保存します：

```cmake
cmake_minimum_required(VERSION 3.14)
project(PyNukoCat)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Torchへのパスを設定
set(Torch_DIR
  "/home/NukoCat/.venv/lib/python3.10/site-packages/torch/share/cmake/Torch"
)

find_package(pybind11 REQUIRED)

find_package(Torch REQUIRED)

file(GLOB_RECURSE SRC
    "../../cpp/core/*.cpp"
    "../../cpp/mcts/*.cpp"
    "../../cpp/nn/*.cpp"
    "main.cpp"
)

pybind11_add_module(nukocat_cpp ${SRC})
target_link_libraries(nukocat_cpp PRIVATE "${TORCH_LIBRARIES}")
target_compile_definitions(nukocat_cpp PRIVATE TORCH_API_INCLUDE_EXTENSION_H)
set_target_properties(nukocat_cpp PROPERTIES
    CXX_STANDARD 17
    PREFIX ""
)
```

⚠️ 注意：

- Torch_DIR は PyTorch のインストール先に合わせて変更してください。

## 2. ビルド手順

ターミナルで以下を実行します

```bash
cd ~/NukoCat/cpp/pybind
rm -rf build
mkdir build && cd build
cmake .. -DPYTHON_EXECUTABLE=$(which python)
make -j4
cd ~/NukoCat
```

正常にビルドが完了すると、build ディレクトリ内に以下のファイルが生成されます：

```bash
nukocat_cpp.cpython-310-x86_64-linux-gnu.so
```

⚠️ 注意：

- .so は Python 3.10 用にビルドされています。他のバージョンでは互換性がありません。
- CMake や pybind11、libtorch のバージョンに応じてパスを調整してください。
