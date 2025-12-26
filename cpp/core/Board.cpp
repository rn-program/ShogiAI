#include "Board.hpp"

#include <cassert>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

// ===============================
// SFEN 分解
// ===============================
sfenParts splitSfen(const std::string &sfen)
{
    std::stringstream ss(sfen);
    sfenParts p;

    if (!(ss >> p.board >> p.turn >> p.hand >> p.moveNumber))
        throw std::runtime_error("Invalid SFEN");

    return p;
}
namespace shogi
{

    // ===============================
    // Bitboard81
    // ===============================
    Bitboard81::Bitboard81() noexcept : lo(0), hi(0) {}
    Bitboard81::Bitboard81(U64 a, U64 b) noexcept : lo(a), hi(b) {}

    void Bitboard81::reset() noexcept
    {
        lo = hi = 0;
    }

    void Bitboard81::set(int sq) noexcept
    {
        assert(0 <= sq && sq < NUM_SQ);
        if (sq < 64)
            lo |= (U64(1) << sq);
        else
            hi |= (U64(1) << (sq - 64));
    }

    void Bitboard81::clear(int sq) noexcept
    {
        assert(0 <= sq && sq < NUM_SQ);
        if (sq < 64)
            lo &= ~(U64(1) << sq);
        else
            hi &= ~(U64(1) << (sq - 64));
    }

    bool Bitboard81::test(int sq) const noexcept
    {
        if (!(0 <= sq && sq < NUM_SQ))
        {
            std::cout << "test sq: " << sq << "\n";
        }
        assert(0 <= sq && sq < NUM_SQ);
        if (sq < 64)
            return (lo >> sq) & 1;
        return (hi >> (sq - 64)) & 1;
    }

    int Bitboard81::popcount() const noexcept
    {
        return __builtin_popcountll(lo) + __builtin_popcountll(hi);
    }

    std::vector<int> Bitboard81::squares() const
    {
        std::vector<int> out;

        U64 a = lo;
        while (a)
        {
            int b = __builtin_ctzll(a);
            out.push_back(b);
            a &= a - 1;
        }

        a = hi;
        while (a)
        {
            int b = __builtin_ctzll(a);
            out.push_back(64 + b);
            a &= a - 1;
        }

        return out;
    }

    Bitboard81 &Bitboard81::operator|=(const Bitboard81 &o) noexcept
    {
        lo |= o.lo;
        hi |= o.hi;
        return *this;
    }

    // ===============================
    // sq ヘルパー
    // ===============================
    int sq(int file, int rank)
    {
        assert(1 <= file && file <= 9);
        assert(1 <= rank && rank <= 9);
        return (rank - 1) * 9 + (file - 1);
    }

    // ===============================
    // Board
    // ===============================
    Board::Board()
    {
        reset();
    }

    void Board::reset() noexcept
    {
        for (auto &bb : pieceBB)
            bb.reset();

        senteHand.clear();
        goteHand.clear();
        turn = Player::Sente;
        moveNumber = 1;
        moveHistory.clear();
    }

    void Board::setInitialPosition()
    {
        setPositionFromSFEN(
            "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1");
    }

    void Board::setPositionFromSFEN(const std::string &sfen)
    {
        reset();

        sfenParts p = splitSfen(sfen);
        std::string board = p.board;

        // 一番左下から
        int rank = 8;
        int file = 0;

        for (size_t i = 0; i < board.size(); i++)
        {
            char c = board[i];

            // 段区切り
            if (c == '/')
            {
                rank--;
                file = 0;
                continue;
            }

            // 数字 → 空マスの数
            if (std::isdigit(c))
            {
                file += (c - '0'); // 例えば '3' → 3 マス進める
                continue;
            }

            // '+' → 成り駒
            bool isPromote = false;
            if (c == '+')
            {
                isPromote = true;
                c = board[++i]; // 次の文字が本当の駒
            }

            // 先手 or 後手
            bool isSente = std::isupper(c);
            char cl = std::tolower(c);

            PieceType pt;

            switch (cl)
            {
            case 'p':
                pt = PieceType::Pawn;
                break;
            case 'l':
                pt = PieceType::Lance;
                break;
            case 'n':
                pt = PieceType::Knight;
                break;
            case 's':
                pt = PieceType::Silver;
                break;
            case 'g':
                pt = PieceType::Gold;
                break;
            case 'b':
                pt = PieceType::Bishop;
                break;
            case 'r':
                pt = PieceType::Rook;
                break;
            case 'k':
                pt = PieceType::King;
                break;
            default:
                continue;
            }

            if (isPromote)
            {
                pt = promote(pt);
            }

            int sq = rank * 9 + file;
            int idx = int(pt) + (isSente ? 0 : 14);
            pieceBB[idx].set(sq);

            file++;
        }

        // 手番設定
        turn = (p.turn == "b" ? Player::Sente : Player::Gote);

        // 持ち駒設定
        if (p.hand != "-")
        {
            int number = 0;

            for (char c : p.hand)
            {
                if (std::isdigit(c))
                {
                    number = number * 10 + (c - '0');
                    continue;
                }

                bool isSente = std::isupper(c);
                char cl = std::tolower(c);

                PieceType pt;

                switch (cl)
                {
                case 'p':
                    pt = PieceType::Pawn;
                    break;
                case 'l':
                    pt = PieceType::Lance;
                    break;
                case 'n':
                    pt = PieceType::Knight;
                    break;
                case 's':
                    pt = PieceType::Silver;
                    break;
                case 'g':
                    pt = PieceType::Gold;
                    break;
                case 'b':
                    pt = PieceType::Bishop;
                    break;
                case 'r':
                    pt = PieceType::Rook;
                    break;
                default:
                    number = 0;
                    continue;
                }

                int count = (number == 0 ? 1 : number);

                if (isSente)
                    senteHand[pt] += count;
                else
                    goteHand[pt] += count;

                number = 0;
            }
        }

        // 手数
        moveNumber = p.moveNumber;
    }

    Bitboard81 Board::occupied() const noexcept
    {
        Bitboard81 out;
        for (const auto &b : pieceBB)
            out |= b;
        return out;
    }

    bool Board::pieceAt(int sq) const noexcept
    {
        return occupied().test(sq);
    }

    PieceType Board::promote(PieceType p) const
    {
        switch (p)
        {
        case PieceType::Pawn:
            return PieceType::ProPawn;
        case PieceType::Lance:
            return PieceType::ProLance;
        case PieceType::Knight:
            return PieceType::ProKnight;
        case PieceType::Silver:
            return PieceType::ProSilver;
        case PieceType::Bishop:
            return PieceType::Horse;
        case PieceType::Rook:
            return PieceType::Dragon;
        default:
            return p;
        }
    }

    // 局面の合法性チェック
    bool Board::isLegal() const
    {
        // 王手放置以外の反則は基本ここまでで防げているはず
        bool is_check = isCheck(turn == Player::Sente ? Player::Gote : Player::Sente);
        if (is_check)
            return false;

        return true;
    }

    // turnの王が王手されているか
    bool Board::isCheck(Player turn) const
    {
        int kingSquare = -1;
        {
            int kingIndex = (turn == Player::Sente)
                                ? int(PieceType::King)
                                : int(PieceType::King) + 14;

            const Bitboard81 &bb = pieceBB[kingIndex];
            if (bb.lo)
            {
                // lo 側（0〜63）
                int b = __builtin_ctzll(bb.lo);
                kingSquare = b;
            }
            else if (bb.hi)
            {
                // hi 側（64〜80）
                int b = __builtin_ctzll(bb.hi);
                kingSquare = 64 + b;
            }
            else
            {
                throw std::runtime_error("玉が盤上に存在しません");
            }
        }

        // 手番側じゃない方の合法手 (この合法手のうち、玉を取る手があれば王手)
        Player miroor_turn = turn == Player::Sente ? Player::Gote : Player::Sente;
        std::vector<Move> moves = generateMoves(miroor_turn);

        for (Move m : moves)
        {
            if (m.to == kingSquare)
                return true;
        }
        return false;
    }

    bool Board::isCheckMate()
    {
        return false;
    }

    std::vector<Move> Board::generateMoves(Player turn) const
    {
        std::vector<Move> moves;
        Bitboard81 occ = occupied();
        bool senteTurn = (turn == Player::Sente);
        int base = senteTurn ? 0 : 14;

        auto isFriend = [&](int sq)
        {
            for (int i = 0; i < 14; i++)
                if (pieceBB[base + i].test(sq))
                    return true;
            return false;
        };
        auto inPromotionZone = [&](int sq) -> bool
        {
            int r = sq / 9;
            return (senteTurn ? (r >= 6) : (r <= 2));
        };

        auto canPromote = [&](PieceType pt, int fromSq, int toSq) -> bool
        {
            if (pt == PieceType::Pawn || pt == PieceType::Lance || pt == PieceType::Knight || pt == PieceType::Silver || pt == PieceType::Bishop || pt == PieceType::Rook)
                return inPromotionZone(fromSq) || inPromotionZone(toSq);
            return false;
        };

        auto mustPromote = [&](PieceType pt, int toSq) -> bool
        {
            int r = toSq / 9;
            if (pt == PieceType::Pawn || pt == PieceType::Lance)
                return (senteTurn ? r == 8 : r == 0);
            if (pt == PieceType::Knight)
                return (senteTurn ? r >= 7 : r <= 1);
            return false;
        };

        // -------- 歩の移動 --------
        for (int fromSq : pieceBB[base + int(PieceType::Pawn)].squares())
        {
            int toSq = fromSq + (senteTurn ? 9 : -9);
            if (toSq >= 0 && toSq < 81 && !isFriend(toSq))
            {
                if (mustPromote(PieceType::Pawn, toSq))
                    moves.push_back(Move{fromSq, toSq, PieceType::Pawn, true, PieceType::None});
                else if (canPromote(PieceType::Pawn, fromSq, toSq))
                {
                    moves.push_back(Move{fromSq, toSq, PieceType::Pawn, false, PieceType::None});
                    moves.push_back(Move{fromSq, toSq, PieceType::Pawn, true, PieceType::None});
                }
                else
                    moves.push_back(Move{fromSq, toSq, PieceType::Pawn, false, PieceType::None});
            }
        }

        // -------- 桂の移動 --------
        const int knightOffsetsSente[] = {17, 19};
        const int knightOffsetsGote[] = {-17, -19};
        const int *knightOffsets = senteTurn ? knightOffsetsSente : knightOffsetsGote;

        for (int fromSq : pieceBB[base + int(PieceType::Knight)].squares())
        {
            for (int i = 0; i < 2; i++)
            {
                int toSq = fromSq + knightOffsets[i];
                if (toSq >= 0 && toSq < 81 && !isFriend(toSq))
                {
                    if (mustPromote(PieceType::Knight, toSq))
                        moves.push_back(Move{fromSq, toSq, PieceType::Knight, true, PieceType::None});
                    else if (canPromote(PieceType::Knight, fromSq, toSq))
                    {
                        moves.push_back(Move{fromSq, toSq, PieceType::Knight, false, PieceType::None});
                        moves.push_back(Move{fromSq, toSq, PieceType::Knight, true, PieceType::None});
                    }
                    else
                        moves.push_back(Move{fromSq, toSq, PieceType::Knight, false, PieceType::None});
                }
            }
        }

        // -------- (銀・金・王)、成駒（歩・香・桂・銀) ----------------
        struct PtOffset
        {
            PieceType pt;
            std::vector<int> offsets;
        };
        std::vector<PtOffset> pieceOffsets = {
            {PieceType::Silver, senteTurn ? std::vector<int>{+9, +8, +10, +1, -1} : std::vector<int>{-9, -8, -10, +1, -1}},
            {PieceType::Gold, senteTurn ? std::vector<int>{+9, +8, +10, +1, -1, -9} : std::vector<int>{-9, -8, -10, +1, -1, +9}},
            {PieceType::ProPawn, senteTurn ? std::vector<int>{+9, +8, +10, +1, -1, -9} : std::vector<int>{-9, -8, -10, +1, -1, +9}},
            {PieceType::ProLance, senteTurn ? std::vector<int>{+9, +8, +10, +1, -1, -9} : std::vector<int>{-9, -8, -10, +1, -1, +9}},
            {PieceType::ProKnight, senteTurn ? std::vector<int>{+9, +8, +10, +1, -1, -9} : std::vector<int>{-9, -8, -10, +1, -1, +9}},
            {PieceType::ProSilver, senteTurn ? std::vector<int>{+9, +8, +10, +1, -1, -9} : std::vector<int>{-9, -8, -10, +1, -1, +9}},
            {PieceType::King, senteTurn ? std::vector<int>{+9, +8, +10, +1, -1, -9, -8, -10} : std::vector<int>{-9, -8, -10, +1, -1, +9, +8, +10}}};

        for (auto &po : pieceOffsets)
        {
            int index = base + int(po.pt);
            for (int fromSq : pieceBB[index].squares())
            {
                for (int dir : po.offsets)
                {
                    int toSq = fromSq + dir;
                    if (toSq >= 0 && toSq < 81 && !isFriend(toSq))
                    {
                        if (canPromote(po.pt, fromSq, toSq))
                        {
                            moves.push_back(Move{fromSq, toSq, po.pt, false, PieceType::None});
                            moves.push_back(Move{fromSq, toSq, po.pt, true, PieceType::None});
                        }
                        else
                            moves.push_back(Move{fromSq, toSq, po.pt, false, PieceType::None});
                    }
                }
            }
        }

        // -------- スライド駒（飛・角・香）、成り駒 (馬・龍) --------
        auto generateSlide = [&](int fromSq, const std::vector<int> &dirs, PieceType pt)
        {
            for (int dir : dirs)
            {
                int toSq = fromSq;
                while (true)
                {
                    int nextSq = toSq + dir;
                    if (nextSq < 0 || nextSq >= 81)
                        break;

                    int toFile = toSq % 9, toRank = toSq / 9;
                    int nextFile = nextSq % 9, nextRank = nextSq / 9;

                    if ((dir == 1 || dir == -1) && nextRank != toRank)
                        break;
                    if ((dir == 9 || dir == -9) && nextFile != toFile)
                        break;
                    if ((dir == 8 || dir == -8 || dir == 10 || dir == -10) && (std::abs(nextFile - toFile) != 1 || std::abs(nextRank - toRank) != 1))
                        break;

                    if (isFriend(nextSq))
                        break;

                    if (mustPromote(pt, nextSq))
                        moves.push_back(Move{fromSq, nextSq, pt, true, PieceType::None});
                    else if (canPromote(pt, fromSq, nextSq))
                    {
                        moves.push_back(Move{fromSq, nextSq, pt, false, PieceType::None});
                        moves.push_back(Move{fromSq, nextSq, pt, true, PieceType::None});
                    }
                    else
                        moves.push_back(Move{fromSq, nextSq, pt, false, PieceType::None});

                    if (occupied().test(nextSq))
                        break;

                    toSq = nextSq;
                }
            }
        };

        // 飛・角・香
        for (int fromSq : pieceBB[base + int(PieceType::Bishop)].squares())
            generateSlide(fromSq, {-10, -8, 8, 10}, PieceType::Bishop);
        for (int fromSq : pieceBB[base + int(PieceType::Rook)].squares())
            generateSlide(fromSq, {-1, 1, -9, 9}, PieceType::Rook);
        for (int fromSq : pieceBB[base + int(PieceType::Lance)].squares())
            generateSlide(fromSq, {senteTurn ? 9 : -9}, PieceType::Lance);

        // 馬・龍
        // --- Horse（馬） --- 斜めスライドは既に追加済み
        for (int fromSq : pieceBB[base + int(PieceType::Horse)].squares())
        {
            // スライド部分
            generateSlide(fromSq, {-10, -8, 8, 10}, PieceType::Horse);

            // 1マス部分
            const std::vector<int> horseSteps = {+1, -1, +9, -9};
            for (int dir : horseSteps)
            {
                int toSq = fromSq + dir;
                if (toSq < 0 || toSq >= 81)
                    continue;

                int f1 = fromSq % 9, r1 = fromSq / 9;
                int f2 = toSq % 9, r2 = toSq / 9;

                // 馬の1マス移動は盤外チェック only（王と同じ）
                if (std::abs(f1 - f2) + std::abs(r1 - r2) != 1)
                    continue;

                if (isFriend(toSq))
                    continue;

                moves.push_back(Move{fromSq, toSq, PieceType::Horse, false, PieceType::None});
            }
        }

        // --- Dragon（龍） --- 縦横スライドは既に追加済み
        for (int fromSq : pieceBB[base + int(PieceType::Dragon)].squares())
        {
            // スライド部分
            generateSlide(fromSq, {-1, 1, -9, 9}, PieceType::Dragon);

            // 1マス部分（斜め）
            const std::vector<int> dragonSteps = {+8, +10, -8, -10};
            for (int dir : dragonSteps)
            {
                int toSq = fromSq + dir;
                if (toSq < 0 || toSq >= 81)
                    continue;

                int f1 = fromSq % 9, r1 = fromSq / 9;
                int f2 = toSq % 9, r2 = toSq / 9;

                // 龍の1マス移動は斜め限定
                if (std::abs(f1 - f2) != 1 || std::abs(r1 - r2) != 1)
                    continue;

                if (isFriend(toSq))
                    continue;

                moves.push_back(Move{fromSq, toSq, PieceType::Dragon, false, PieceType::None});
            }
        }

        // -------- 打ち駒生成 --------
        const std::map<PieceType, int> &hand = senteTurn ? senteHand : goteHand;

        for (auto &[pt, cnt] : hand)
        {
            if (cnt <= 0)
                continue;

            for (int sq = 0; sq < 81; sq++)
            {
                if (occupied().test(sq))
                    continue;

                // 二歩禁止：同列に歩がある場合は打てない
                if (pt == PieceType::Pawn)
                {
                    int file = sq % 9;
                    bool hasPawnInFile = false;
                    for (int r = 0; r < 9; r++)
                    {
                        int sqCheck = r * 9 + file;
                        if (pieceBB[base + int(PieceType::Pawn)].test(sqCheck))
                        {
                            hasPawnInFile = true;
                            break;
                        }
                    }
                    if (hasPawnInFile)
                        continue;
                }

                moves.push_back(Move{-1, sq, PieceType::None, false, pt});
            }
        }

        return moves;
    }

    std::vector<Move> Board::generateLegalMoves(Player turn) const
    {
        std::vector<Move> moves = generateMoves(turn);

        moves.erase(
            std::remove_if(
                moves.begin(),
                moves.end(),
                [&](Move m)
                {
                    Board temp = *this;
                    temp.applyMove(m);
                    return temp.isCheck(turn);
                }),
            moves.end());

        return moves;
    }

    void Board::applyMove(Move &m)
    {
        if (m.from == -1)
        {
            // 打ち駒
            if (turn == Player::Sente)
            {
                pieceBB[int(m.drop)].set(m.to);
                senteHand[m.drop]--;
                m.gotPiece = PieceType::None;
            }
            else
            {
                pieceBB[int(m.drop) + 8].set(m.to);
                goteHand[m.drop]--;
                m.gotPiece = PieceType::None;
            }
        }
        else
        {
            if (turn == Player::Sente)
            {
                // 移動先の駒が何もない時
                if (!occupied().test(m.to))
                {
                    pieceBB[int(m.piece)].clear(m.from);
                    PieceType finalPiece = m.promote ? promote(m.piece) : m.piece;
                    pieceBB[int(finalPiece)].set(m.to);
                    m.gotPiece = PieceType::None;
                }
                // 移動先の駒を持ち駒に追加
                else
                {
                    // 取られる駒は後手の駒
                    for (int i = 14; i < 28; i++)
                    {
                        if (pieceBB[i].test(m.to))
                        {
                            m.gotPiece = pieceTypeFromInt(i - 14);
                            senteHand[m.gotPiece]++;
                        }
                    }
                    // 元の駒を消す
                    pieceBB[int(m.piece)].clear(m.from);
                    PieceType finalPiece = m.promote ? promote(m.piece) : m.piece;
                    pieceBB[int(finalPiece)].set(m.to);
                }
            }
            // 後手の時
            else
            {
                // 移動先の駒が何もない時
                if (!occupied().test(m.to))
                {
                    pieceBB[int(m.piece) + 14].clear(m.from);
                    PieceType finalPiece = m.promote ? promote(m.piece) : m.piece;
                    pieceBB[int(finalPiece) + 14].set(m.to);
                    m.gotPiece = PieceType::None;
                }
                // 移動先の駒を持ち駒に追加
                else
                {
                    // 取られる駒は先手の駒
                    for (int i = 0; i < 14; i++)
                    {
                        if (pieceBB[i].test(m.to))
                        {
                            m.gotPiece = pieceTypeFromInt(i);
                            goteHand[m.gotPiece]++;
                        }
                    }
                    // 元の駒を消す
                    pieceBB[int(m.piece) + 14].clear(m.from);
                    PieceType finalPiece = m.promote ? promote(m.piece) : m.piece;
                    pieceBB[int(finalPiece) + 14].set(m.to);
                }
            }
        }
        // 棋譜追加
        lastMove = m;
        turn = turn == Player::Sente ? Player::Gote : Player::Sente;
        moveNumber++;
        moveHistory.push_back(m);
    }
    void Board::undoMove()
    {
        Move &m = lastMove;
        PieceType piece = m.piece;

        // 最終手で成った場合の駒名変換
        if (m.promote)
        {
            piece = promote(m.piece);
        }

        // 先手の時 (後手の着手を変更)
        if (turn == Player::Sente)
        {
            // 最後の着手が駒打ちの時
            if (m.from == -1)
            {
                pieceBB[int(m.drop) + 14].clear(m.to);
                goteHand[m.drop]++;
            }
            // 最後の着手が駒移動かつ駒を取っていない時
            else if (m.gotPiece == PieceType::None)
            {
                pieceBB[int(piece) + 14].clear(m.to);
                pieceBB[int(m.piece) + 14].set(m.from);
            }
            // 最後の駒移動で駒を取ったとき
            else
            {
                pieceBB[int(piece) + 14].clear(m.to);
                pieceBB[int(m.piece) + 14].set(m.from);
                // 元々あった駒を置く
                pieceBB[int(m.gotPiece)].set(m.to);
                goteHand[m.gotPiece]--;
            }
        }
        // 後手の時 (先手の着手を変更)
        else
        {
            // 最後の着手が駒打ちの時
            if (m.from == -1)
            {
                pieceBB[int(m.drop)].clear(m.to);
                senteHand[m.drop]++;
            }
            // 最後の着手が駒移動かつ駒を取っていない時
            else if (m.gotPiece == PieceType::None)
            {
                pieceBB[int(piece)].clear(m.to);
                pieceBB[int(m.piece)].set(m.from);
            }
            // 最後の駒移動で駒を取ったとき
            else
            {
                pieceBB[int(piece)].clear(m.to);
                pieceBB[int(m.piece)].set(m.from);
                // 元々あった駒を置く
                pieceBB[int(m.gotPiece) + 14].set(m.to);
                senteHand[m.gotPiece]--;
            }
        }
        turn = turn == Player::Sente ? Player::Gote : Player::Sente;
        moveNumber--;
        moveHistory.pop_back();
        lastMove = moveHistory.back();
    }

    Move Board::fromUSI(const std::string &usi, Player turn)
    {
        Move m;
        int base = (turn == Player::Sente ? 0 : 14);

        // =====================
        // 持ち駒打ち
        // =====================
        if (usi.size() == 4 && usi[1] == '*')
        {
            int toFile = '9' - usi[2];
            int toRank = 'i' - usi[3];

            m.from = -1; // 打ちは from 無効
            m.to = toRank * 9 + toFile;
            m.promote = false;

            // 打つ駒
            m.drop = pieceTypeFromChar(usi[0]);
            m.piece = PieceType::None;

            return m;
        }

        // =====================
        // 通常の駒移動
        // =====================
        if (usi.size() == 4 || usi.size() == 5)
        {
            int fromFile = '9' - usi[0];
            int fromRank = 'i' - usi[1];
            int toFile = '9' - usi[2];
            int toRank = 'i' - usi[3];

            m.from = fromRank * 9 + fromFile;
            m.to = toRank * 9 + toFile;

            // 移動した駒の特定
            for (int i = 0; i < 14; i++)
            {
                if (pieceBB[base + i].test(m.from))
                {
                    m.piece = pieceTypeFromInt(i);
                    break;
                }
            }

            // 成り
            m.promote = (usi.size() == 5 && usi[4] == '+');
            m.drop = PieceType::None;
            return m;
        }

        throw std::runtime_error("Invalid USI");
    }

    void Board::applyUSI(const std::string &usi)
    {
        Move m = fromUSI(usi, turn);
        applyMove(m);
    }

    void Board::debugPrint() const
    {
        const char *pieceChar[28] = {
            "P", "+P", "L", "+L", "N", "+N", "S", "+S", "G", "B", "+B", "R", "+R", "K",
            "p", "+p", "l", "+l", "n", "+n", "s", "+s", "g", "b", "+b", "r", "+r", "k"};

        std::cout << "現在の局面" << std::endl;
        // 局面部分の表示
        for (int r = 8; r >= 0; --r)
        {
            for (int f = 0; f < 9; ++f)
            {
                int sq = r * 9 + f;
                const char *out = ".";
                for (int i = 0; i < 28; i++)
                {
                    if (pieceBB[i].test(sq))
                    {
                        out = pieceChar[i]; // ← 文字列で出す
                        break;
                    }
                }
                std::cout << out << " "; // 文字が2文字なのでスペースで区切る
            }
            std::cout << std::endl;
        }

        for (const auto &[pt, count] : senteHand)
        {
            std::cout << "先手の持ち駒: " << std::endl;
            std::cout << pieceNameJP(pt) << " : " << count << std::endl;
        }

        for (const auto &[pt, count] : goteHand)
        {
            std::cout << "後手の持ち駒: " << std::endl;
            std::cout << pieceNameJP(pt) << " : " << count << std::endl;
        }
    }

} // namespace shogi
