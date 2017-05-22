#pragma once

#include <iostream>
#include "CommonConstants.h"

namespace reversi
{
    class Board
    {
    public:
        Board();
        Board(const Board& board_);
        ~Board();

        int getCeilColor(int index) const;
        bool getPlayerColor() const;

        bool isMove(bool player);
        bool isGame();
        bool isCeilPossible(int index, bool player);
        bool isCeilStable(int index) const;

        bool setCeil(int index);
        int getValue();
    private:
        bool switchPlayer();
        bool checkMove(int index, bool player, int direction, int depth, bool isToSet);
        bool checkStability(int index, bool player, int direction) const;

        bool playerColor; // цвет игрока

        bool tableOfWhite[64]; // таблица с белыми
        bool tableOfBlack[64]; // таблица с чёрными
    };
}