#include <string>
#include <iostream>
#include <vector>
#include "Game.h"

using namespace reversi;


int main()
{
    Reversi reversi;
    std::string str;
    while (std::cin >> str) {
        if (str == "init") { // инициализация
            std::string color;
            std::cin >> color;
        }
        if (str == "turn") { // пора сделать ход
            int move = reversi.callAIMove();
            int x = (move / 8) + 1;
            char y = (char)(move % 8) + 'a';
            std::cout << "move " << y << " " << x << std::endl;
        }
        if (str == "move") {
            int x;
            char y;
            std::cin >> y >> x;
            --x;
            int index = 8 * x + (y - 'a');
            reversi.setCeil(index);
        }
        if (str == "bad" || str == "lose" || str == "win" || str == "draw") {
            break;
        }
    }
    return 0;
}