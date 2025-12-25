#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>
#include <cstdint>

#include "Move.hpp"
#include "Piece.hpp"
#include "Player.hpp"

struct sfenParts
{
    std::string board; // 例: "lnsgkgsnl/1r5b1/p1ppppppp/.../LNSGKGSNL"
    std::string turn;  // "b" または "w"
    std::string hand;  // 例: "R2B" など、無い場合は "-"
    int moveNumber;    // 手数
};

namespace shogi
{
    using U64 = uint64_t;

    struct Bitboard81
    {
        U64 lo;
        U64 hi;

        Bitboard81() noexcept;
        explicit Bitboard81(U64 a, U64 b = 0) noexcept;

        static constexpr int NUM_SQ = 81;

        void reset() noexcept;
        void set(int sq) noexcept;
        void clear(int sq) noexcept;
        bool test(int sq) const noexcept;
        int popcount() const noexcept;
        std::vector<int> squares() const;
        Bitboard81 &operator|=(const Bitboard81 &o) noexcept;
    };

    int sq(int file, int rank);

    class Board
    {
    public:
        std::array<Bitboard81, 28> pieceBB;
        std::map<PieceType, int> senteHand;
        std::map<PieceType, int> goteHand;
        Player turn;
        Move lastMove;
        std::vector<Move> moveHistory;
        int moveNumber;

        Board();

        void reset() noexcept;
        void setInitialPosition();
        void setPositionFromSFEN(const std::string &sfen);

        Bitboard81 occupied() const noexcept;
        bool pieceAt(int sq) const noexcept;

        void applyMove(Move &m);
        void undoMove();

        Move fromUSI(const std::string &usi, Player turn);
        void applyUSI(const std::string &usi);

        PieceType promote(PieceType p) const;

        bool isLegal() const;
        bool isCheck(Player p) const;
        bool isCheckMate();

        std::vector<Move> generateMoves(Player turn) const;
        std::vector<Move> generateLegalMoves(Player turn) const;

        void debugPrint() const;
    };
}
