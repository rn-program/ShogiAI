import argparse

from kivy.app import App
from kivy.uix.gridlayout import GridLayout
from kivy.uix.floatlayout import FloatLayout
from kivy.uix.relativelayout import RelativeLayout
from kivy.uix.image import Image
from kivy.uix.behaviors import ButtonBehavior
from kivy.uix.button import Button
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.label import Label
from kivy.graphics import Color, Rectangle, Line
from kivy.clock import Clock

from threading import Thread

import shogi
from predict import predict_best_move_mcts

# -------------------------
# グローバル
# -------------------------
selectedPiece = None  # 盤上の駒選択
selectedHand = None  # 持ち駒選択
holding_pieces = {0: [], 1: []}  # 0=先手,1=後手

parser = argparse.ArgumentParser(description="模倣AI設定")
parser.add_argument("--username", type=str, required=True)
parser.add_argument("--ai_turn", type=int, default=1)
parser.add_argument("--simulations", type=int, default=1000)

args = parser.parse_args()

username = args.username
ai_turn = args.ai_turn
simulations = args.simulations

# -------------------------
# 駒画像
# -------------------------
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


# -------------------------
# USI / SFEN
# -------------------------
def CoorsToUSI(col, row):
    colToAlpha = {
        0: "a",
        1: "b",
        2: "c",
        3: "d",
        4: "e",
        5: "f",
        6: "g",
        7: "h",
        8: "i",
    }
    return str(9 - col) + colToAlpha[row]


def board_to_piece_list(board):
    sfen = board.sfen()
    piece_list = []
    board_part = sfen.split()[0]

    for row in board_part.split("/"):
        row_list = []
        i = 0
        while i < len(row):
            c = row[i]
            if c.isdigit():
                row_list.extend(["."] * int(c))
                i += 1
            elif c == "+":
                i += 1
                row_list.append("+" + row[i])
                i += 1
            else:
                row_list.append(c)
                i += 1

        while len(row_list) < 9:
            row_list.append(".")
        piece_list.append(row_list)

    return piece_list


def board_to_holding_pieces(board):
    global holding_pieces
    holding_pieces = {0: [], 1: []}

    sfen_parts = board.sfen().split()
    if len(sfen_parts) < 3:
        return

    hand = sfen_parts[2]
    if hand == "-":
        return

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


# -------------------------
# ボード初期化
# -------------------------
board = shogi.Board()
# sfenで局面指定
board.set_sfen("lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1")
piece_list = board_to_piece_list(board)


# -------------------------
# 持ち駒ボタン（枚数表示対応）
# -------------------------
class HoldingPieceButton(RelativeLayout):
    def __init__(self, piece, owner, count=1, **kwargs):
        super().__init__(**kwargs)
        self.piece = piece
        self.owner = owner
        self.size_hint = (None, None)
        self.width = self.height = 50

        # 駒画像
        self.img = Image(source=piece_images[piece], size_hint=(1, 1))
        self.add_widget(self.img)

        # 枚数ラベル
        self.count_label = Label(
            text=str(count) if count > 1 else "",
            size_hint=(None, None),
            size=(20, 20),
            pos=(self.width - 20, self.height - 20),
            color=(1, 1, 1, 1),
            bold=True,
        )
        self.add_widget(self.count_label)
        self.bind(size=self.update_label_pos)

    def update_label_pos(self, *args):
        self.count_label.pos = (
            self.width - self.count_label.width,
            self.height - self.count_label.height,
        )

    def on_touch_down(self, touch):
        if self.collide_point(*touch.pos):
            global selectedHand, selectedPiece, turn
            if (turn == 0 and self.owner == 0) or (turn == 1 and self.owner == 1):
                selectedHand = self.piece
                selectedPiece = None
                print(f"[DEBUG] 持ち駒 {self.piece} を選択")
                return True
        return super().on_touch_down(touch)


# -------------------------
# 盤上駒ボタン
# -------------------------
class PieceButton(ButtonBehavior, Image):
    def __init__(self, row, col, source=None, **kwargs):
        super().__init__(**kwargs)
        self.row = row
        self.col = col
        self.piece = None
        self.highlighted = False
        self.promotion_buttons = []
        self.size_hint = (None, None)
        self.size = (50, 50)

        if source:
            self.source = source

        with self.canvas.before:
            if (row + col) % 2 == 0:
                Color(0.95, 0.85, 0.7, 1)
            else:
                Color(0.85, 0.7, 0.5, 1)
            self.rect = Rectangle(pos=self.pos, size=self.size)

        self.highlight_line = None
        self.bind(pos=self.update_rect, size=self.update_rect)

    def update_rect(self, *args):
        self.rect.pos = self.pos
        self.rect.size = self.size
        if self.highlight_line:
            self.highlight_line.rectangle = (
                self.pos[0],
                self.pos[1],
                self.size[0],
                self.size[1],
            )

    def on_press(self):
        global selectedPiece, selectedHand, turn
        col, row = self.col, self.row
        self.piece = piece_list[row][col]

        # --- 持ち駒を打つ ---
        if selectedHand and self.piece == ".":
            usi_move = selectedHand.upper()
            usi = usi_move + "*" + CoorsToUSI(col, row)
            legal_usi = [m.usi() for m in board.legal_moves]
            print(usi, legal_usi)
            if usi in legal_usi:
                move = shogi.Move.from_usi(usi)
                board.push(move)
                holder = 0 if turn == 0 else 1
                holding_pieces[holder].remove(selectedHand)
                selectedHand = None

                update_board_and_buttons()

                # 非同期でAI呼び出し
                if turn == ai_turn and not ai_thinking:
                    Clock.schedule_once(
                        lambda dt: Thread(target=ai_move, daemon=True).start(), 0
                    )
                print(f"[DEBUG] 持ち駒打ち: {move.usi()}")
                print(f"[DEBUG] 現在のSFEN: {board.sfen()}")
            else:
                print("[DEBUG] 反則打ち")
            return

        # --- 盤上駒選択／移動 ---
        if self.piece == "." and selectedPiece is None:
            return

        if selectedPiece is None and (
            (turn == 0 and self.piece.islower()) or (turn == 1 and self.piece.isupper())
        ):
            print("[DEBUG] 選択できない駒です")
            return

        if self.highlighted:
            self.remove_highlight()
            selectedPiece = None
        else:
            for btn in app_ref.piece_buttons:
                btn.remove_highlight()
                btn.remove_promotion_buttons()

            if selectedPiece is not None:
                departure, destination = selectedPiece, CoorsToUSI(col, row)
                usi_normal = departure + destination
                usi_promote = departure + destination + "+"
                legal_usi = [m.usi() for m in board.legal_moves]
                normal_legal = usi_normal in legal_usi
                promote_legal = usi_promote in legal_usi

                if normal_legal and promote_legal:
                    self.show_promotion_buttons(departure, destination)
                    return
                elif promote_legal:
                    move_to_play = shogi.Move.from_usi(usi_promote)
                elif normal_legal:
                    move_to_play = shogi.Move.from_usi(usi_normal)
                else:
                    print(f"[DEBUG] 反則手: {usi_normal}")
                    selectedPiece = None
                    return

                handle_capture(move_to_play)

                board.push(move_to_play)
                update_board_and_buttons()

                # 非同期でAI呼び出し
                if turn == ai_turn and not ai_thinking:
                    Clock.schedule_once(
                        lambda dt: Thread(target=ai_move, daemon=True).start(), 0
                    )
                print(f"[DEBUG] 駒移動: {move_to_play.usi()}")
                print(f"[DEBUG] 現在のSFEN: {board.sfen()}")
                selectedPiece = None
            else:
                selectedPiece = CoorsToUSI(col, row)
                self.add_highlight()

    # --- 成ボタン ---
    def show_promotion_buttons(self, departure, destination):
        self.remove_promotion_buttons()
        btn_size = self.size[0] / 2
        win_x, win_y = self.to_window(self.x, self.y)

        btn_promote = Button(
            text="成",
            font_size=18,
            font_name="static/font/NotoSansJP-Regular.ttf",
            size_hint=(None, None),
            size=(btn_size, btn_size * 0.7),
            pos=(win_x + self.width - btn_size, win_y + self.height - btn_size * 0.7),
            background_color=(1, 0.6, 0.6, 1),
        )
        btn_normal = Button(
            text="不成",
            font_size=18,
            font_name="static/font/NotoSansJP-Regular.ttf",
            size_hint=(None, None),
            size=(btn_size, btn_size * 0.7),
            pos=(win_x, win_y + self.height - btn_size * 0.7),
            background_color=(0.6, 0.8, 1, 1),
        )

        def promote_action(instance):
            global selectedPiece
            move = shogi.Move.from_usi(departure + destination + "+")
            handle_capture(move)
            board.push(move)
            self.remove_promotion_buttons()
            update_board_and_buttons()
            # 非同期でAI呼び出し
            if turn == ai_turn and not ai_thinking:
                Clock.schedule_once(
                    lambda dt: Thread(target=ai_move, daemon=True).start(), 0
                )
            print(f"[DEBUG] 成り移動: {move.usi()}")
            print(f"[DEBUG] 現在のSFEN: {board.sfen()}")
            selectedPiece = None
            for btn in app_ref.piece_buttons:
                btn.remove_highlight()

        def normal_action(instance):
            global selectedPiece
            move = shogi.Move.from_usi(departure + destination)
            handle_capture(move)
            board.push(move)
            self.remove_promotion_buttons()
            update_board_and_buttons()
            # 非同期でAI呼び出し
            if turn == ai_turn and not ai_thinking:
                Clock.schedule_once(
                    lambda dt: Thread(target=ai_move, daemon=True).start(), 0
                )
            print(f"[DEBUG] 不成移動: {move.usi()}")
            print(f"[DEBUG] 現在のSFEN: {board.sfen()}")
            selectedPiece = None
            for btn in app_ref.piece_buttons:
                btn.remove_highlight()

        btn_promote.bind(on_press=promote_action)
        btn_normal.bind(on_press=normal_action)
        app_ref.promotion_layer.add_widget(btn_promote)
        app_ref.promotion_layer.add_widget(btn_normal)
        self.promotion_buttons.extend([btn_promote, btn_normal])

    def remove_promotion_buttons(self):
        for b in self.promotion_buttons:
            if b.parent:
                b.parent.remove_widget(b)
        self.promotion_buttons = []

    def add_highlight(self):
        self.highlighted = True
        with self.canvas.after:
            Color(1, 0, 0, 1)
            self.highlight_line = Line(
                rectangle=(self.pos[0], self.pos[1], self.size[0], self.size[1]),
                width=3,
            )

    def remove_highlight(self):
        if self.highlighted and self.highlight_line:
            self.canvas.after.remove(self.highlight_line)
            self.highlight_line = None
            self.highlighted = False


# -------------------------
# キャプチャ処理
# -------------------------
def handle_capture(move):
    captured_piece = board.piece_at(move.to_square)
    if captured_piece:
        holder = 0 if board.turn == shogi.BLACK else 1
        piece_symbol = captured_piece.symbol().replace("+", "")
        piece_symbol = piece_symbol.upper() if holder == 0 else piece_symbol.lower()
        holding_pieces[holder].append(piece_symbol)


# -------------------------
# 持ち駒更新
# -------------------------
def update_holding_area():
    app_ref.top_captures.clear_widgets()
    app_ref.bottom_captures.clear_widgets()

    top_height = app_ref.top_captures.height
    bottom_height = app_ref.bottom_captures.height

    display_order = ["P", "L", "N", "S", "G", "B", "R"]

    for owner in [1, 0]:
        holder_pieces = holding_pieces[owner]
        counts = {}
        for p in holder_pieces:
            counts[p] = counts.get(p, 0) + 1

        for base_piece in display_order:
            piece = base_piece.lower() if owner == 1 else base_piece
            if piece in counts:
                c = counts[piece]
                btn = HoldingPieceButton(piece=piece, owner=owner, count=c)
                btn.height = btn.width = (
                    top_height * 0.9 if owner == 1 else bottom_height * 0.9
                )
                if owner == 1:
                    app_ref.top_captures.add_widget(btn)
                else:
                    app_ref.bottom_captures.add_widget(btn)


# -------------------------
# AIを別スレッドで実行
# -------------------------
ai_thinking = False


def ai_move():
    global ai_thinking
    ai_thinking = True

    print("[DEBUG] AI思考中...")

    best_move_str = predict_best_move_mcts(board.sfen(), username, simulations)
    if best_move_str is None:
        return

    Clock.schedule_once(lambda dt: apply_ai_move(best_move_str), 0)


def apply_ai_move(best_move_str):
    global ai_thinking

    move = best_move_str
    best_move_usi = best_move_str.usi()

    legal_usi = [m.usi() for m in board.legal_moves]
    if not legal_usi:
        ai_thinking = False
        return

    if best_move_usi in legal_usi:
        move_usi = best_move_str

    # 反則手だった場合、合法手リストの最初の手を指す
    else:
        move_usi = legal_usi[0]
        print(f"[DEBUG] AIの反則手検知: {best_move_str} → {move_usi}")

        move = shogi.Move.from_usi(move_usi)

    print(f"[DEBUG] AIの指し手: {move.usi()}")
    board.push(move)

    update_board_and_buttons()
    ai_thinking = False


# -------------------------
# 盤更新
# -------------------------
def update_board_and_buttons(trigger_ai=True):
    global turn, piece_list
    turn = 0 if board.turn == shogi.BLACK else 1
    board_to_holding_pieces(board)
    piece_list[:] = board_to_piece_list(board)

    for btn in app_ref.piece_buttons:
        piece = piece_list[btn.row][btn.col]
        btn.source = "static/image/empty.png" if piece == "." else piece_images[piece]

    update_holding_area()


# -------------------------
# アプリ本体
# -------------------------
class ShogiApp(App):
    def build(self):
        global app_ref
        app_ref = self
        root = FloatLayout()

        # 上持ち駒
        app_ref.top_captures = BoxLayout(
            size_hint=(1, 0.12),
            pos_hint={"top": 1},
            spacing=5,
            padding=[5, 0, 5, 0],
        )
        with app_ref.top_captures.canvas.before:
            Color(0.9, 0.9, 0.9, 1)
            app_ref.top_bg = Rectangle(
                pos=app_ref.top_captures.pos, size=app_ref.top_captures.size
            )
        app_ref.top_captures.bind(pos=self.update_top_bg, size=self.update_top_bg)
        root.add_widget(app_ref.top_captures)

        # 盤 + 成ボタンレイヤ
        app_ref.promotion_layer = FloatLayout(size_hint=(1, 0.76), pos_hint={"y": 0.12})

        # 盤
        app_ref.board_layout = GridLayout(
            cols=9, rows=9, spacing=2, size_hint=(None, None)
        )
        app_ref.piece_buttons = []
        for row in range(9):
            for col in range(9):
                piece = piece_list[row][col]
                btn = PieceButton(
                    row,
                    col,
                    source=(
                        "static/image/empty.png"
                        if piece == "."
                        else piece_images[piece]
                    ),
                )
                app_ref.board_layout.add_widget(btn)
                app_ref.piece_buttons.append(btn)

        app_ref.promotion_layer.add_widget(app_ref.board_layout)
        root.add_widget(app_ref.promotion_layer)

        # 下持ち駒
        app_ref.bottom_captures = BoxLayout(
            size_hint=(1, 0.12),
            pos_hint={"y": 0},
            spacing=5,
            padding=[5, 0, 5, 0],
        )
        with app_ref.bottom_captures.canvas.before:
            Color(0.9, 0.9, 0.9, 1)
            app_ref.bottom_bg = Rectangle(
                pos=app_ref.bottom_captures.pos, size=app_ref.bottom_captures.size
            )
        app_ref.bottom_captures.bind(
            pos=self.update_bottom_bg, size=self.update_bottom_bg
        )
        root.add_widget(app_ref.bottom_captures)

        update_board_and_buttons()
        root.bind(size=self.update_board)
        self.update_board(root, root.size)
        return root

    def update_top_bg(self, *args):
        app_ref.top_bg.pos = app_ref.top_captures.pos
        app_ref.top_bg.size = app_ref.top_captures.size

    def update_bottom_bg(self, *args):
        app_ref.bottom_bg.pos = app_ref.bottom_captures.pos
        app_ref.bottom_bg.size = app_ref.bottom_captures.size

    def update_board(self, instance, size):
        available_height = size[1] * 0.76
        board_size = min(size[0], available_height)
        app_ref.board_layout.size = (board_size, board_size)
        btn_size = board_size / 9
        for btn in app_ref.piece_buttons:
            btn.size = (btn_size, btn_size)
        x_offset = (size[0] - board_size) / 2
        y_offset = (available_height - board_size) / 2 + size[1] * 0.12
        app_ref.board_layout.pos = (x_offset, y_offset)

        # 駒台幅を画面幅いっぱい
        app_ref.top_captures.width = size[0]
        app_ref.bottom_captures.width = size[0]
        app_ref.top_captures.pos = (0, size[1] - app_ref.top_captures.height)
        app_ref.bottom_captures.pos = (0, 0)


if __name__ == "__main__":
    try:
        ShogiApp().run()
    except KeyboardInterrupt:
        print("終了コマンドを検知")
