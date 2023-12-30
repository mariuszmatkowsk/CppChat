#include <asio.hpp>
#include <format>
#include <iostream>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <variant>

#include "Channel/channel.hpp"

constexpr std::string_view hello_msg =
    "Hello, you are now connected to the server...";

struct ClientConnected {
    std::shared_ptr<asio::ip::tcp::socket> socket;
};

struct ClientDisconnected {
    asio::ip::tcp::endpoint endpoint;
};

struct NewMessage {
    asio::ip::tcp::endpoint endpoint;
    std::string message;
};

using Message = std::variant<ClientConnected, ClientDisconnected, NewMessage>;
using Clients = std::unordered_map<asio::ip::tcp::endpoint,
                                   std::shared_ptr<asio::ip::tcp::socket>>;

class MessageHandler {
public:
    MessageHandler(Clients &clients) : clients_(clients) {}

    void operator()(const ClientConnected &client_connected_msg) {
        const auto &socket = client_connected_msg.socket;

        std::cout << std::format(
            "New connection from: {}:{}\n",
            socket->remote_endpoint().address().to_string(),
            socket->remote_endpoint().port());

        if (clients_.find(socket->remote_endpoint()) == std::end(clients_)) {
            client_connected_msg.socket->write_some(
                asio::buffer(hello_msg.data(), hello_msg.length()));
            clients_.insert(std::make_pair(socket->remote_endpoint(), socket));
        }
    }

    void operator()(const ClientDisconnected &client_disconnected_msg) {}

    void operator()(const NewMessage &new_message) {}

private:
    Clients &clients_;
};

template <typename T>
void client(std::shared_ptr<asio::ip::tcp::socket> soc,
            mpsc::Sender<T> sender) {

    sender.send(ClientConnected{soc});

    while (true) {
    }
}

template <typename T> void server(mpsc::Receiver<T> receiver) {
    Clients clients;

    MessageHandler message_handler(clients);

    while (true) {
        if (auto message = receiver.recv()) {
            // if (std::holds_alternative<ClientConnected>(*message)) {
            //     const auto& client_connected =
            //     std::get<ClientConnected>(*message);
            //     // Handle client connected
            //     //
            // } else if (std::holds_alternative<ClientDisconnected>(*message))
            // {
            //     const auto& client_disconnected =
            //     std::get<ClientDisconnected>(*message);
            //     // Handle Client Disconnected
            // } else {
            //     const auto& new_message =
            //     std::get<ClientDisconnected>(*message);
            //     // Handle NewMessage
            // }
            std::visit(message_handler, *message);
        }
    }
}

int main() {
    asio::io_context ioc;
    asio::error_code ec;

    asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), 6969);

    asio::ip::tcp::acceptor acceptor(ioc, endpoint);

    auto [sender, receiver] = mpsc::make_channel<Message>();

    std::thread(server<Message>, std::move(receiver)).detach();

    while (true) {
        asio::ip::tcp::socket socket(ioc);
        acceptor.accept(socket);
        auto soc_ptr =
            std::make_shared<asio::ip::tcp::socket>(std::move(socket));
        std::thread(client<Message>, std::move(soc_ptr), sender).detach();
    }

    return 0;
}
