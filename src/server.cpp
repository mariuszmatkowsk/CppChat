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

constexpr std::string_view disconnect_msg = "You are now disconnected...";

struct ClientConnected {
    std::shared_ptr<asio::ip::tcp::socket> socket;
};

struct ClientDisconnected {
    asio::ip::tcp::endpoint endpoint;
};

struct NewMessage {
    asio::ip::tcp::endpoint endpoint;
    std::vector<uint8_t> message;
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

    void operator()(const ClientDisconnected &client_disconnected_msg) {
        std::cout << "Handle client disconnection in server function\n";
        auto it = clients_.find(client_disconnected_msg.endpoint);
        if (it != std::end(clients_)) {
            const auto &socket = it->second;

            std::cout << std::format(
                "Client with address {}:{} has been disconnected\n",
                it->first.address().to_string(), it->first.port());

            clients_.erase(it);
        }
    }

    void operator()(const NewMessage &new_message) {}

private:
    Clients &clients_;
};

template <typename T>
void client(std::shared_ptr<asio::ip::tcp::socket> socket,
            mpsc::Sender<T> sender) {

    sender.send(ClientConnected{socket});

    constexpr size_t BUFFER_SIZE = 1024;
    std::vector<uint8_t> buffer(BUFFER_SIZE);
    while (true) {
        asio::error_code ec;
        auto n = asio::read(*socket, asio::buffer(buffer, BUFFER_SIZE), ec);

        if (n == 0) {
            sender.send(ClientDisconnected{socket->remote_endpoint()});
            return;
        } else {
            // TODO: New message to be handled
        }
    }
}

template <typename T> void server(mpsc::Receiver<T> receiver) {
    Clients clients;
    MessageHandler message_handler(clients);

    while (true) {
        if (auto message = receiver.recv()) {
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
