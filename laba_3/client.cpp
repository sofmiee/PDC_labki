#include "client.hpp"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/connect.hpp>
#include <iostream>

using boost::asio::ip::tcp;

ClientApp::ClientApp(const std::string& host, unsigned short port) : socket_(io_) {
    tcp::resolver resolver(io_);
    auto endpoints = resolver.resolve(host, std::to_string(port));
    boost::asio::connect(socket_, endpoints);
}

void ClientApp::run() {
    std::cout << "Connected. Enter requests. Type quit to exit." << std::endl;
    start_read();
    io_thread_ = std::thread([this]() { io_.run(); });

    std::string line;
    while (running_ && std::getline(std::cin, line)) {
        bool is_quit = (line == "quit");
        send_line(std::move(line));
        if (is_quit) {
            break;
        }
    }

    stop();
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

void ClientApp::start_read() {
    boost::asio::async_read_until(
        socket_, reply_buffer_, '\n',
        [this](const boost::system::error_code& ec, std::size_t bytes) { handle_read(ec, bytes); });
}

void ClientApp::handle_read(const boost::system::error_code& ec, std::size_t) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted && ec != boost::asio::error::eof) {
            std::cerr << "Read error: " << ec.message() << std::endl;
        }
        running_ = false;
        stop();
        return;
    }

    std::istream in(&reply_buffer_);
    std::string line;
    std::getline(in, line);
    std::cout << line << std::endl;
    start_read();
}

void ClientApp::send_line(std::string line) {
    if (!running_) {
        return;
    }
    line.push_back('\n');
    boost::asio::post(io_, [this, line = std::move(line)]() mutable {
        bool idle = outgoing_.empty();
        outgoing_.push_back(std::move(line));
        if (idle && !write_in_progress_) {
            do_write();
        }
    });
}

void ClientApp::do_write() {
    if (outgoing_.empty()) {
        write_in_progress_ = false;
        return;
    }
    write_in_progress_ = true;
    boost::asio::async_write(socket_, boost::asio::buffer(outgoing_.front()),
                             [this](const boost::system::error_code& ec, std::size_t) {
                                 if (ec) {
                                     if (ec != boost::asio::error::operation_aborted) {
                                         std::cerr << "Write error: " << ec.message() << std::endl;
                                     }
                                     running_ = false;
                                     stop();
                                     return;
                                 }
                                 outgoing_.pop_front();
                                 do_write();
                             });
}

void ClientApp::stop() {
    if (!running_) {
        boost::system::error_code ignored;
        socket_.close(ignored);
        io_.stop();
        return;
    }
    running_ = false;
    boost::system::error_code ignored;
    socket_.shutdown(tcp::socket::shutdown_both, ignored);
    socket_.close(ignored);
    io_.stop();
}

int main(int argc, char* argv[]) {
    try {
        std::string host = "127.0.0.1";
        unsigned short port = 12345;
        if (argc > 1) host = argv[1];
        if (argc > 2) port = static_cast<unsigned short>(std::stoi(argv[2]));
        ClientApp app(host, port);
        app.run();
    } catch (const std::exception& ex) {
        std::cerr << "Client error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
