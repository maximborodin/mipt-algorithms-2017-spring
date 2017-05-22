#pragma once

#include <iostream>
#include <ctime>
#include "CommonConstants.h"
#include "Board.h"

namespace reversi
{
    class Reversi
    {
    public:
        Reversi();
        ~Reversi();


        bool isGame();

        bool setCeil(int index);

        int callAIMove();
        void search();

    private:
        int miniMax(int maxDepth);
        int miniMax(Board* node, int depth, const int maxDepth, int alpha, int beta);

        Board* board;
        time_t startTime;

        int curBestMove;
        int bestMove;
    };
}