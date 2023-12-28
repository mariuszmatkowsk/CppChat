#include <iostream>
#include <ncurses.h>

#define ctrl(x) (x & 0x1F)

int main() {
    initscr();
    raw();
    noecho();
    std::cout << "Hello from client\n";

    bool quit = false;

    while (!quit) {
        switch (auto ch = getch()) {
            case ctrl('c'):
                quit = true;
                break;
            default:
                break;
        }
    }


    return 0;
}
