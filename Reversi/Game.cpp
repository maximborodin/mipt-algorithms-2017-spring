#include "Game.h"


namespace reversi
{
    Reversi::Reversi() :
        board(new Board)
    {
    }

    Reversi::~Reversi()
    {
        delete board;
    }

    /*
    Идёт ли ещё игра?
    */
    bool Reversi::isGame()
    {
        return board->isGame();
    }

    /*
    Походить в эту клетку
    */
    bool Reversi::setCeil(int index)
    {
        return board->setCeil(index);
    }

    /*
    Просит компьютер сделать ход, возвращает его координаты
    */
    int Reversi::callAIMove()
    {
        if (!isGame()) {
            return -1;
        }
        search();
        board->setCeil(bestMove);
        return bestMove;
    }

    /*
    Ищем наиболее полезный ход
    */
    void Reversi::search()
    {
        startTime = time(NULL);
        for (int depth = 1; depth < MAX_DEPTH; ++depth) {
            int value = miniMax(depth);
            if (time(NULL) - TIME_LIMIT >= startTime) {
                break;
            }
            bestMove = curBestMove;
            //std::cout << "depth " << x << ": "<< value  << ", Best move: " << curBestMove << std::endl;
        }
        //std::cout << "Best Move: " << bestMove << std::endl;
    }

    /*
    Запускаем минимакс
    */
    int Reversi::miniMax(int maxDepth)
    {
        return miniMax(board, 0, maxDepth, -INT_MAX, INT_MAX);
    }

    /*
    Минимакс с отсечением некоторых веток
    */
    int Reversi::miniMax(Board* node, int depth, const int maxDepth, int alpha, int beta)
    {
        if (time(NULL) - TIME_LIMIT >= startTime) { // если время подошло к концу, то нужно заканчивать
            return NULL;
        }
        if (depth >= maxDepth || !node->isGame()) { // если получили невозможный вариант или зашли слишком глубоко
            return node->getValue();                // то возвращаем текущее значение
        }
        int value;
        bool curPlayerColor;
        Board* child;
        /*
        * обрабатываем текущее состояние
        */
        for (int x = 0; x < 64; ++x) {
            if (node->isCeilPossible(x, node->getPlayerColor())) {
                child = new Board(*node);
                curPlayerColor = child->getPlayerColor();
                child->setCeil(x);

                if (child->getPlayerColor() != curPlayerColor) { //если нечетная глубина
                    value = -miniMax(child, depth + 1, maxDepth, -beta, -alpha);
                }
                else {                                         //если чётная глубина
                    value = miniMax(child, depth + 1, maxDepth, alpha, beta);
                }
                delete child;

                if (value >= beta) { // текущее значение каким-то образом стало больше, чем максимум
                    return beta;     // то есть просто максимум
                }
                if (value > alpha) { // обновили результат
                    alpha = value;
                    if (depth == 0) {
                        curBestMove = x;
                    }
                }
            }
            if (time(NULL) - TIME_LIMIT >= startTime) { // если время подошло к концу, то эту глубину не рассматриваем
                return NULL;
            }
        }
        return alpha;
    }
}