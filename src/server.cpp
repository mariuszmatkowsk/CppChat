#include <asio.hpp>
#include <thread>
#include <iostream>
#include <variant>
#include <unordered_map>

#include "Channel/channel.hpp"

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

template <typename T>
void client(std::shared_ptr<asio::ip::tcp::socket> soc, mpsc::Sender<T> sender) {
    std::cout << "New connection established.\n";

    // sender.send(ClientConnected{soc});

    std::string hello_msg = "Hello, you are now connected to the server.";

    soc->write_some(asio::buffer(hello_msg.data(), hello_msg.length()));

    while(true) {


    }
}

template <typename T>
void server(mpsc::Receiver<T> receiver) {
    std::unordered_map<asio::ip::tcp::endpoint, asio::ip::tcp::socket> clients;

    // while(true) {
    //     if (auto message = receiver.try_recv()) {
    //
    //         
    //     }
    // }

}

int main() {
    asio::io_context ioc;
    asio::error_code ec;

    asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), 6969);

    asio::ip::tcp::acceptor acceptor(ioc, endpoint);

    auto [sender, receiver] = mpsc::make_channel<Message>();

    std::thread(server<Message>, std::move(receiver)).detach();

    while(true) {
        asio::ip::tcp::socket socket(ioc);
        acceptor.accept(socket);
        auto soc_ptr = std::make_shared<asio::ip::tcp::socket>(std::move(socket));
        std::thread(client<Message>, std::move(soc_ptr), sender).detach();
    }

    return 0;
}
