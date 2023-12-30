#include <iostream>
#include <ncurses.h>

#include "ScreenState/ScreenState.hpp"

#define ctrl(x) (x & 0x1F)

const short REGULAR_PAIR = 0;
const short HIGHLIGHT_PAIR = 1;

void status_bar(const std::string& label, int x, int y, int width,
                short color_pair) {
    auto n = label.length() - 1;
    auto WHITE_SPACE_N = width - n - 1;

    std::string spaces(WHITE_SPACE_N, ' ');

    attron(COLOR_PAIR(color_pair));
    move(y, x);
    printw(label.c_str(), "%s");
    printw(spaces.c_str(), "%s");

    attroff(COLOR_PAIR(color_pair));
}

int main() {
    auto screen_state = ScreenState::enable();

    init_pair(REGULAR_PAIR, COLOR_WHITE, COLOR_BLACK);
    init_pair(HIGHLIGHT_PAIR, COLOR_BLACK, COLOR_WHITE);

    int x, y;
    getmaxyx(stdscr, y, x);

    status_bar("CppChat", 0, 0, x, HIGHLIGHT_PAIR);

    status_bar("Offline", 0, y - 2, x, HIGHLIGHT_PAIR);

    bool quit = false;

    while (!quit) {
        switch (auto ch = getch()) {
        case ctrl('c'):
            quit = true;
            break;
        default:
            break;
        }
        refresh();
    }

    return 0;
}
