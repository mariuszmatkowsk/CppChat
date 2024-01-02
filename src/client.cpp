#include <algorithm>
#include <array>
#include <asio.hpp>
#include <cctype>
#include <ncurses.h>
#include <string>
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
    void put(char ch) {
        data_.insert(std::begin(data_) + cursor_, ch);
        cursor_++;
    }

    void backspace() {
        if (cursor_ == 0)
            return;
        data_.erase(std::begin(data_) + cursor_ - 1);
        cursor_--;
    }

    void move_cursor_left() {
        cursor_ = cursor_ == 0 ? 0 : cursor_ - 1;
    }

    void move_cursor_right() {
        cursor_ = cursor_ == data_.size() ? cursor_ : cursor_ + 1;
    }

    void clear() {
        data_.clear();
        cursor_ = 0;
    }

    std::string getPromptString() const {
        return {std::begin(data_), std::end(data_)};
    }

    void sync_cursor_with_terminal(unsigned x, unsigned y, unsigned width) {
        const auto cursor_pos = std::min(x + cursor_, width);
        move(y, cursor_pos);
    }

    void render(unsigned x, unsigned y, unsigned width) {
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
    unsigned cursor_;
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
    std::unique_ptr<asio::ip::tcp::socket> socket;
    ChatLog chat;
    bool quit = false;
};

struct Command {
    std::string name;
    std::string description;
    std::string signature;
    std::function<void(Client&, const std::string&)> run;
};

void connect_client(Client& client, const std::string& address) {
    asio::io_context ioc;
    // TODO: handle error when address has wrong format, or is empty
    asio::ip::tcp::endpoint endpoint(asio::ip::address_v4::from_string(address),
                                     6969);
    asio::ip::tcp::socket socket(ioc);
    socket.connect(endpoint);
    client.socket = std::make_unique<asio::ip::tcp::socket>(std::move(socket));
}

void disconnect_client(Client& client, const std::string& = "") {
    client.socket->close();
    client.socket.reset();
}

void quit(Client& client, const std::string& = "") {
    client.quit = true;
}

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

std::optional<std::pair<std::string, std::string>>
parse_prompt(std::string prompt) {
    auto prefix_pos = prompt.find('/');
    if (prefix_pos != 0) {
        return std::nullopt;
    }

    // remove '/' from string
    prompt.erase(0, 1);

    auto space_pos = prompt.find(' ');

    if (space_pos != std::string::npos) {
        return std::pair(prompt.substr(0, space_pos),
                         prompt.substr(space_pos + 1));
    }
    // There is no argument, return whole string
    return std::pair(prompt, std::string{});
}

const std::array<Command, 3> COMMANDS = {
    Command{"connect", "Connect to a server by <ip>", "/connect <ip>",
            connect_client},
    Command{"disconnect", "Disconnect from the server", "/disconnect",
            disconnect_client},
    Command{"quit", "Close the chat", "/quit", quit}};

const Command *find_command(const std::string& name) {
    auto it = std::find_if(
        std::begin(COMMANDS), std::end(COMMANDS),
        [&name](const auto& command) { return command.name == name; });
    if (it == std::end(COMMANDS)) {
        return nullptr;
    }
    return &(*it);
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
                    quit(client);
                    break;
                case ctrl('h'):
                    prompt.move_cursor_left();
                    break;
                case ctrl('l'):
                    prompt.move_cursor_right();
                    break;
                case 127:
                case KEY_BACKSPACE:
                    prompt.backspace();
                    break;
                case '\n': // Handle Error Key
                {
                    auto prompt_msg = prompt.getPromptString();
                    const auto parsed_prompt = parse_prompt(prompt_msg);

                    if (parsed_prompt) {
                        const auto [command, argument] = *parsed_prompt;
                        const auto command_ptr = find_command(command);
                        if (command_ptr) {
                            command_ptr->run(client, argument);
                        } else {
                            err_msg(client.chat, "Unknown command");
                        }
                    } else {
                        msg(client.chat, prompt.getPromptString());
                    }
                    prompt.clear();
                    break;
                }
                default:
                    if (std::isprint(ch)) {
                        prompt.put(ch);
                    }
            }
        }

        status_bar("CppChat", 0, 0, x, Color::Highlight);

        client.chat.render(0, 1);

        if (client.socket)
            status_bar("Online", 0, y - 2, x, Color::Highlight);
        else
            status_bar("Offline", 0, y - 2, x, Color::Highlight);

        prompt.render(0, y - 1, x);
        prompt.sync_cursor_with_terminal(0, y - 1, x);

        refresh();
    }

    return 0;
}
