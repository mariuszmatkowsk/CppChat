#pragma once

#include <ncurses.h>

class ScreenState {
public:
    ScreenState() = default;

    ScreenState& operator=(const ScreenState&) = delete;
    ScreenState& operator=(ScreenState&&) = delete;
    ScreenState(const ScreenState&) = delete;
    ScreenState(ScreenState&&) = delete;

    static ScreenState enable() {
        initscr();
        noecho();
        curs_set(0); // Cursor invisible
        cbreak();
        start_color();
        return ScreenState{};
    }

    ~ScreenState() { endwin(); }
};

