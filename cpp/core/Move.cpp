#include "core/Move.hpp"

#include <iostream>
#include <cctype>
#include <vector>

#include "core/Board.hpp"
#include "core/Player.hpp"

std::string Move::toKIF() const
{
    int file = 9 - to % 9; // 1~9
    int rank = 9 - to / 9; // 上段が1、下段が9

    // 通常移動の時
    if (drop == PieceType::None)
    {
        if (promote)
            return std::to_string(file) + std::to_string(rank) + pieceNameJP(piece) + "成";
        else
            return std::to_string(file) + std::to_string(rank) + pieceNameJP(piece);
    }
    // 持ち駒打ちの時
    else
    {
        return std::to_string(file) + std::to_string(rank) + pieceNameJP(drop) + "打";
    }
};

std::string Move::toUSI(Player turn) const
{
    int from_file = 9 - from % 9; // 1~9
    int to_file = 9 - to % 9;
    int from_rank = 9 - from / 9;
    int to_rank = 9 - to / 9;

    std::vector<std::string> rank_vector = {"a", "b", "c", "d", "e", "f", "g", "h", "i"};

    // 通常移動の時
    if (drop == PieceType::None)
    {
        if (promote)
            return std::to_string(from_file) + rank_vector[from_rank - 1] + std::to_string(to_file) + rank_vector[to_rank - 1] + "+";
        else
            return std::to_string(from_file) + rank_vector[from_rank - 1] + std::to_string(to_file) + rank_vector[to_rank - 1];
    }
    // 持ち駒打ちの時
    else
    {
        std::string piece_usi = pieceNameUSI(drop);
        if (turn == Player::Sente)
        {
            piece_usi[0] = std::toupper(piece_usi[0]);
        }
        return piece_usi + "*" + std::to_string(to_file) + rank_vector[to_rank - 1];
    }
};

PieceType pieceTypeFromChar(char c)
{
    char cl = std::tolower(c);

    switch (cl)
    {
    case 'p':
        return PieceType::Pawn;
    case 'l':
        return PieceType::Lance;
    case 'n':
        return PieceType::Knight;
    case 's':
        return PieceType::Silver;
    case 'g':
        return PieceType::Gold;
    case 'b':
        return PieceType::Bishop;
    case 'r':
        return PieceType::Rook;
    default:
        return PieceType::None;
    }
}