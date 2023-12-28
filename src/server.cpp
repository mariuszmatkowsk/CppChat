#include <iostream>
#include <asio.hpp>

int main() {
    asio::io_context context;
    asio::error_code ec;

    asio::ip::tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1", ec), 6969);

    std::cout << "Hello from server\n";

    return 0;
}
