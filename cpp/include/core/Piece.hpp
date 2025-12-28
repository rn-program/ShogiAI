#pragma once

#include <string>

#include "Player.hpp"

// ---------------------------
// 駒の種類
// ---------------------------
enum class PieceType
{
    Pawn,
    ProPawn,
    Lance,
    ProLance,
    Knight,
    ProKnight,
    Silver,
    ProSilver,
    Gold,
    Bishop,
    Horse,
    Rook,
    Dragon,
    King,
    None
};

// ---------------------------
// ヘルパー関数（宣言）
// ---------------------------
PieceType pieceTypeFromInt(int x);
std::string pieceNameJP(PieceType pt);
std::string pieceNameUSI(PieceType pt);
