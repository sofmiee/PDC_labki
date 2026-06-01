#include "server.hpp"

#include <boost/asio.hpp>
#include <csignal>
#include <iostream>
#include <string>

namespace {
    AsyncServer* g_async_server = nullptr;

    void on_signal(int) {
        if (g_async_server != nullptr) {
            g_async_server->stop();
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        boost::asio::io_context io;
        std::string mode = "async";
        unsigned short port = 12345;
        std::size_t threads = 4;

        if (argc > 1) mode = argv[1];
        if (argc > 2) port = static_cast<unsigned short>(std::stoi(argv[2]));
        if (argc > 3) threads = static_cast<std::size_t>(std::stoul(argv[3]));

        if (mode == "sync") {
            SyncCalculatorServer server(io, port);
            std::cout << "Sync calculator server on 127.0.0.1:" << port << std::endl;
            server.run();
            return 0;
        }

        AsyncServer server(io, port, threads);
        g_async_server = &server;
        std::signal(SIGINT, on_signal);
        std::signal(SIGTERM, on_signal);

        std::cout << "Async server on 127.0.0.1:" << port << ", threads: " << threads << std::endl;
        std::cout << "Commands: calc, average list, integer factorial, remind <sec> <text>, quit" << std::endl;
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}