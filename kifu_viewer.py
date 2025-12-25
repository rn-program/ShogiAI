import json
import argparse

import shogi

from kivy.app import App
from kivy.uix.floatlayout import FloatLayout
from kivy.uix.gridlayout import GridLayout
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.image import Image
from kivy.uix.label import Label
from kivy.uix.button import Button
from kivy.uix.relativelayout import RelativeLayout

# =========================
# 駒画像
# =========================
piece_images = {
    "P": "static/image/black_pawn.png",
    "L": "static/image/black_lance.png",
    "N": "static/image/black_knight.png",
    "S": "static/image/black_silver.png",
    "G": "static/image/black_gold.png",
    "K": "static/image/black_king.png",
    "R": "static/image/black_rook.png",
    "B": "static/image/black_bishop.png",
    "+P": "static/image/black_prom_pawn.png",
    "+R": "static/image/black_prom_rook.png",
    "+B": "static/image/black_prom_bishop.png",
    "+S": "static/image/black_prom_silver.png",
    "+N": "static/image/black_prom_knight.png",
    "+L": "static/image/black_prom_lance.png",
    "p": "static/image/white_pawn.png",
    "l": "static/image/white_lance.png",
    "n": "static/image/white_knight.png",
    "s": "static/image/white_silver.png",
    "g": "static/image/white_gold.png",
    "k": "static/image/white_king.png",
    "r": "static/image/white_rook.png",
    "b": "static/image/white_bishop.png",
    "+p": "static/image/white_prom_pawn.png",
    "+r": "static/image/white_prom_rook.png",
    "+b": "static/image/white_prom_bishop.png",
    "+s": "static/image/white_prom_silver.png",
    "+n": "static/image/white_prom_knight.png",
    "+l": "static/image/white_prom_lance.png",
}


# =========================
# SFEN → 盤配列
# =========================
def board_to_piece_list(board):
    board_part = board.sfen().split()[0]
    rows = []
    for row in board_part.split("/"):
        r = []
        i = 0
        while i < len(row):
            c = row[i]
            if c.isdigit():
                r.extend(["."] * int(c))
                i += 1
            elif c == "+":
                r.append("+" + row[i + 1])
                i += 2
            else:
                r.append(c)
                i += 1
        rows.append(r)
    return rows


# =========================
# 持ち駒復元
# =========================
holding_pieces = {0: [], 1: []}


def board_to_holding_pieces(board):
    global holding_pieces
    holding_pieces = {0: [], 1: []}

    parts = board.sfen().split()
    if len(parts) < 3 or parts[2] == "-":
        return

    hand = parts[2]
    i = 0
    while i < len(hand):
        if hand[i].isdigit():
            count = int(hand[i])
            i += 1
            piece = hand[i]
        else:
            count = 1
            piece = hand[i]

        owner = 0 if piece.isupper() else 1
        for _ in range(count):
            holding_pieces[owner].append(piece)
        i += 1


# =========================
# 持ち駒表示
# =========================
class HoldingPieceButton(RelativeLayout):
    def __init__(self, piece, count, **kwargs):
        super().__init__(**kwargs)
        self.size_hint = (None, None)
        self.size = (48, 48)

        self.add_widget(Image(source=piece_images[piece], size_hint=(1, 1)))

        if count > 1:
            self.add_widget(
                Label(
                    text=str(count),
                    size_hint=(None, None),
                    size=(18, 18),
                    pos=(30, 30),
                    color=(1, 1, 1, 1),
                )
            )


# =========================
# メインGUI
# =========================
class KifuPlayerApp(App):
    def __init__(self, moves, **kwargs):
        super().__init__(**kwargs)
        self.moves = moves
        self.current_ply = 0
        self.board = shogi.Board()
        self.board.reset()

    def build(self):
        root = FloatLayout()

        # 上持ち駒（後手）
        self.top_captures = BoxLayout(
            size_hint=(1, 0.1),
            pos_hint={"top": 1},
            spacing=4,
            padding=4,
        )
        root.add_widget(self.top_captures)

        # 盤
        self.board_layout = GridLayout(cols=9, rows=9, size_hint=(None, None))
        self.piece_imgs = []
        for _ in range(81):
            img = Image(source="static/image/empty.png")
            self.board_layout.add_widget(img)
            self.piece_imgs.append(img)
        root.add_widget(self.board_layout)

        # 下持ち駒（先手）
        self.bottom_captures = BoxLayout(
            size_hint=(1, 0.1),
            pos_hint={"y": 0},
            spacing=4,
            padding=4,
        )
        root.add_widget(self.bottom_captures)

        # コントロール
        ctrl = BoxLayout(
            size_hint=(1, 0.1),
            pos_hint={"y": 0.1},
            spacing=10,
            padding=10,
        )

        ctrl.add_widget(Button(text="Prev", on_press=self.prev_move))
        self.info = Label(text="0 / 0")
        ctrl.add_widget(self.info)
        ctrl.add_widget(Button(text="Next", on_press=self.next_move))

        root.add_widget(ctrl)

        root.bind(size=self.on_resize)
        self.on_resize(root, root.size)
        self.refresh()

        return root

    def on_resize(self, _, size):
        board_size = min(size[0], size[1] * 0.7)
        self.board_layout.size = (board_size, board_size)
        self.board_layout.pos = (
            (size[0] - board_size) / 2,
            size[1] * 0.15,
        )
        btn_size = board_size / 9
        for img in self.piece_imgs:
            img.size = (btn_size, btn_size)

    # =====================
    # 再生制御
    # =====================
    def next_move(self, *_):
        if self.current_ply >= len(self.moves):
            return
        self.board.push_usi(self.moves[self.current_ply]["move"])
        self.current_ply += 1
        self.refresh()

    def prev_move(self, *_):
        if self.current_ply == 0:
            return
        self.board.reset()
        for i in range(self.current_ply - 1):
            self.board.push_usi(self.moves[i]["move"])
        self.current_ply -= 1
        self.refresh()

    # =====================
    # 盤・持ち駒更新
    # =====================
    def refresh(self):
        pieces = board_to_piece_list(self.board)
        board_to_holding_pieces(self.board)

        for i, img in enumerate(self.piece_imgs):
            r = i // 9
            c = i % 9
            p = pieces[r][c]
            img.source = "static/image/empty.png" if p == "." else piece_images[p]

        self.top_captures.clear_widgets()
        self.bottom_captures.clear_widgets()

        for owner in [1, 0]:
            counts = {}
            for p in holding_pieces[owner]:
                counts[p] = counts.get(p, 0) + 1
            for piece, cnt in counts.items():
                btn = HoldingPieceButton(piece, cnt)
                if owner == 1:
                    self.top_captures.add_widget(btn)
                else:
                    self.bottom_captures.add_widget(btn)

        self.info.text = f"{self.current_ply} / {len(self.moves)}"


# =========================
# エントリポイント
# =========================
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--kifu", required=True, help="played_moves.json")
    args = parser.parse_args()

    with open(args.kifu, "r", encoding="utf-8") as f:
        data = json.load(f)
    
    moves = data["moves"]

    KifuPlayerApp(moves).run()
