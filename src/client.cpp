#include <cctype>
#include <iostream>
#include <ncurses.h>
#include <vector>

#include "ScreenState/ScreenState.hpp"

#define ctrl(x) (x & 0x1F)

enum Color : short {
    Regular = 0,
    Highlight,
    Error,
    Info,
};

class Prompt {
public:
    void put(char ch) { data_.push_back(ch); }

    void clear() { data_.clear(); }

    std::string getPromptString() const {
        return {std::begin(data_), std::end(data_)};
    }

    void render(int x, int y, int width) {
        auto start_x = data_.size() + x;
        auto WHITE_SPACE_N = width - start_x;

        std::string spaces(WHITE_SPACE_N, ' ');

        attron(COLOR_PAIR(Color::Regular));
        if (!data_.empty()) {
            move(y, x);
            printw(std::string(std::begin(data_), std::end(data_)).c_str(),
                   "%s");
        }
        move(y, start_x);
        printw(spaces.c_str(), "%s");
        attroff(COLOR_PAIR(Color::Regular));
    }

private:
    std::vector<char> data_;
};

class ChatLog {
public:
    void put(std::string message, Color color = Color::Regular) {
        items_.push_back(std::make_pair(std::move(message), color));
    }

    void render(int x, int y) {
        for (unsigned offset = 0; const auto& [message, color] : items_) {
            attron(COLOR_PAIR(color));
            move(y + offset, x);
            printw(message.c_str(), "%s");
            attroff(COLOR_PAIR(color));
            offset++;
        }
    }

private:
    std::vector<std::pair<std::string, Color>> items_;
};

inline void msg(ChatLog& chat, std::string message) {
    chat.put(std::move(message), Color::Regular);
}

inline void err_msg(ChatLog& chat, std::string message) {
    chat.put(std::move(message), Color::Error);
}

inline void info_msg(ChatLog& chat, std::string message) {
    chat.put(std::move(message), Color::Info);
}

struct Client {
    ChatLog chat;
    bool quit = false;
};

void status_bar(const std::string& label, int x, int y, int width,
                Color color) {
    auto n = label.length() - 1;
    auto WHITE_SPACE_N = width - n - 1;

    std::string spaces(WHITE_SPACE_N, ' ');

    attron(COLOR_PAIR(color));
    move(y, x);
    printw(label.c_str(), "%s");
    printw(spaces.c_str(), "%s");

    attroff(COLOR_PAIR(color));
}

int main() {
    Client client{};
    Prompt prompt{};

    auto _screen_state = ScreenState::enable();
    nodelay(stdscr, true);

    init_pair(Color::Regular, COLOR_WHITE, COLOR_BLACK);
    init_pair(Color::Highlight, COLOR_BLACK, COLOR_WHITE);
    init_pair(Color::Error, COLOR_RED, COLOR_BLACK);
    init_pair(Color::Info, COLOR_BLUE, COLOR_BLACK);

    int x, y;
    getmaxyx(stdscr, y, x);

    while (!client.quit) {
        auto ch = getch();
        if (ch != ERR) {
            switch (ch) {
                case ctrl('c'):
                    client.quit = true;
                    break;
                case KEY_DC: // Handle backspace
                case 127:
                case KEY_BACKSPACE:
                    // TODO: handle backspace

                    break;
                case '\n': // Handle Error Key
                    msg(client.chat, prompt.getPromptString());
                    prompt.clear();
                    break;
                default:
                    if (std::isprint(ch)) {
                        prompt.put(ch);
                    }
            }
        }

        status_bar("CppChat", 0, 0, x, Color::Highlight);
        client.chat.render(0, 1);
        status_bar("Offline", 0, y - 2, x, Color::Highlight);
        prompt.render(0, y - 1, x);

        refresh();
    }

    return 0;
}
