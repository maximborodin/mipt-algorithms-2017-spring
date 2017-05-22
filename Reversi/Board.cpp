#include "Board.h"


namespace reversi {
    Board::Board() :
        playerColor(BLACK) // первым ходит чёрный
    {
        for (int x = 0; x < 64; ++x) {
            tableOfWhite[x] = false;
            tableOfBlack[x] = false;
        }
        tableOfWhite[27] = true;//белые клетки вначале
        tableOfWhite[36] = true;
        tableOfBlack[28] = true;//черные клетки вначале
        tableOfBlack[35] = true;
    }

    Board::Board(const Board& board_) :
        playerColor(board_.playerColor) {
        for (int x = 0; x < 64; ++x) {
            tableOfWhite[x] = board_.tableOfWhite[x];
            tableOfBlack[x] = board_.tableOfBlack[x];
        }
    }

    Board::~Board() {

    }

    /*
    Получить цвет выбранной клетки
    */
    int Board::getCeilColor(int index) const {
        if (tableOfWhite[index]) {
            return WHITE_CEIL;
        }
        if (tableOfBlack[index]) {
            return BLACK_CEIL;
        }
        return EMPTY_CEIL;
    }

    /*
    Получить цвет текущего игрока
    */
    bool Board::getPlayerColor() const {
        return playerColor;
    }

    /*
    Есть ли ещё ход?
    */
    bool Board::isMove(bool player) {
        for (int x = 0; x < 64; ++x) {
            if (isCeilPossible(x, player)) {
                return true;
            }
        }
        return false;
    }

    /*
    Идёт ли игра?
    */
    bool Board::isGame() {
        return isMove(BLACK) || isMove(WHITE);
    }

    /*
    Можно ли сходить в эту клетку?
    */
    bool Board::isCeilPossible(int index, bool player) {
        if (getCeilColor(index) != EMPTY_CEIL) {
            return false;
        }

        for (int x = 0; x < 8; ++x) {
            if (checkMove(index, player, x, 0, false)) {
                return true;
            }
        }
        return false;
    }

    /*
    Проверяет клетку на стабильность по 4-м направлениям
    */
    bool Board::isCeilStable(int index) const {
        int ceilColor = getCeilColor(index);
        if (ceilColor == EMPTY_CEIL) {
            return false;
        }
        bool curPlayerColor;
        if (ceilColor == WHITE_CEIL) {
            curPlayerColor = WHITE;
        }
        else {
            curPlayerColor = BLACK;
        }

        for (int x = 0; x < 4; ++x) {
            if (!(checkStability(index, curPlayerColor, x) ||
                checkStability(index, curPlayerColor, x + 4))) {
                return false;
            }
        }
        return true;
    }
    /*
    * Посчитаем функцию от текущего состояния
    */
    int Board::getValue() {
        if (!isGame()) {
            int whiteCount = 0;
            int blackCount = 0;
            for (int x = 0; x < 64; ++x) {
                if (getCeilColor(x) == WHITE_CEIL) {
                    ++whiteCount;
                }
                else if (getCeilColor(x) == BLACK_CEIL) {
                    ++blackCount;
                }
            }
            if (blackCount > whiteCount) {
                return playerColor == BLACK ? MAX_VALUE : -MAX_VALUE;
            }
            else if (blackCount < whiteCount) {
                return playerColor == WHITE ? MAX_VALUE : -MAX_VALUE;
            }
            return 0;
        }

        int mobility = 0; // количество клеток, куда можно ставить
        int stable = 0; // количество стабильных фишек с нужными весами
        int tableValue = 0; // количество фишек нужного цвета на доске

        int ceilColor;
        if (playerColor == BLACK) {
            ceilColor = BLACK_CEIL;
        }
        else {
            ceilColor = WHITE_CEIL;
        }
        int currentCeilColor;

        for (int x = 0; x < 64; ++x) {
            currentCeilColor = getCeilColor(x);
            if (isCeilPossible(x, playerColor)) {
                ++mobility;
            }
            if (isCeilPossible(x, !playerColor)) {
                --mobility;
            }
            if (isCeilStable(x)) {
                if (currentCeilColor == ceilColor) {
                    stable += PRIORITIES_TABLE[x];
                }
                else {
                    stable -= PRIORITIES_TABLE[x];
                }
            }
            if (currentCeilColor) {
                if (currentCeilColor == ceilColor) {
                    tableValue += PRIORITIES_TABLE[x];
                }
                else {
                    tableValue -= PRIORITIES_TABLE[x];
                }
            }
        }
        return mobility * MOBILITY_WEIGHT + stable * STABLE_WEIGHT + tableValue * TABLE_WEIGHT;
    }

    /*
    * Поставить фишку в данную ячейку, если это возможно
    */
    bool Board::setCeil(int index) {
        bool isPossible = false;
        for (int x = 0; x < 8; x++)
            if (checkMove(index, playerColor, x, 0, true)) {
                isPossible = true;
            }

        if (isPossible) {
            tableOfWhite[index] = playerColor == WHITE;
            tableOfBlack[index] = playerColor == BLACK;
            switchPlayer();
        }
        return isPossible;
    }

    /*
    сменить ходящего игрока, если текущий не имеет хода
    */
    bool Board::switchPlayer() {
        if (isMove(!playerColor)) {
            playerColor = !playerColor;
            return true;
        }
        return false;
    }

    /*
    Проверит, можно ли сходить в эту клетку
    depth - глубина обхода в данном направлении
    isToSet - нужно ли ставить
    Если обнаружит, что можно, то рекурсивно вызовется от клеток
    в том же направлении и перевернёт фишки
    */
    bool Board::checkMove(int index, bool player, int direction, int depth, bool isToSet) {
        int x = index % 8 + X_OFFSET[direction];
        int y = index / 8 + Y_OFFSET[direction];
        if (x < 0 || x >= 8 || y < 0 || y >= 8) {
            return false;
        }

        index = 8 * y + x;

        if (getCeilColor(index) == EMPTY_CEIL) {
            return false;
        }
        if (getCeilColor(index) == (player == WHITE ? WHITE_CEIL : BLACK_CEIL)) {
            return depth > 0;
        }

        bool isPossible = checkMove(index, player, direction, depth + 1, isToSet);

        if (isToSet && isPossible) {
            if (player == WHITE) {
                tableOfWhite[index] = true;
                tableOfBlack[index] = false;
            }
            else {
                tableOfWhite[index] = false;
                tableOfBlack[index] = true;
            }
        }
        return isPossible;
    }

    /*
    Проверяем, что все клетки в выбранном направлении нужного цвета
    */
    bool Board::checkStability(int index, bool player, int direction) const {
        int x = index % 8 + X_OFFSET[direction];
        int y = index / 8 + Y_OFFSET[direction];
        if (x < 0 || x >= 8 || y < 0 || y >= 8) {
            return true;
        }
        int newIndex = 8 * y + x;
        /*
        * все клетки в выбранном направлении того же цвета. То есть перевернуть данную фишку уже не получится
        */
        if (getCeilColor(newIndex) == (player == BLACK ? BLACK_CEIL : WHITE_CEIL)) {
            return checkStability(newIndex, player, direction);
        }
        return false;
    }
}