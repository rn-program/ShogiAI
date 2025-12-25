#include "Piece.hpp"

#include <iostream>

PieceType pieceTypeFromInt(int x)
{
    if (x < 0 || x > static_cast<int>(PieceType::None))
    {
        throw std::out_of_range("Invalid piece type");
    }
    return static_cast<PieceType>(x);
}

std::string pieceNameJP(PieceType pt)
{
    switch (pt)
    {
    case PieceType::Pawn:
        return "歩";
    case PieceType::ProPawn:
        return "と";

    case PieceType::Lance:
        return "香";
    case PieceType::ProLance:
        return "成香";

    case PieceType::Knight:
        return "桂";
    case PieceType::ProKnight:
        return "成桂";

    case PieceType::Silver:
        return "銀";
    case PieceType::ProSilver:
        return "成銀";

    case PieceType::Gold:
        return "金";

    case PieceType::Bishop:
        return "角";
    case PieceType::Horse:
        return "馬";

    case PieceType::Rook:
        return "飛";
    case PieceType::Dragon:
        return "竜";

    case PieceType::King:
        return "玉";

    case PieceType::None:
        return "None";
    default:
        return "";
    }
}

std::string pieceNameUSI(PieceType pt)
{
    switch (pt)
    {
    case PieceType::Pawn:
        return "p";

    case PieceType::Lance:
        return "l";

    case PieceType::Knight:
        return "k";

    case PieceType::Silver:
        return "s";

    case PieceType::Gold:
        return "g";

    case PieceType::Bishop:
        return "b";

    case PieceType::Rook:
        return "r";

    case PieceType::King:
        return "k";

    case PieceType::None:
        return "None";
    default:
        return "";
    }
}
