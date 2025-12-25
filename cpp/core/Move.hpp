#pragma once

#include <string>
#include "Piece.hpp"
#include "Player.hpp"

struct Move
{
    int from;           // 元マス (-1 は打ち駒)
    int to;             // 移動先
    PieceType piece;    // 駒種類
    bool promote;       // 成り
    PieceType drop;     // 打ち駒種類
    PieceType gotPiece; // 最後に取った駒

    std::string toKIF() const;
    std::string toUSI(Player turn) const;
};
