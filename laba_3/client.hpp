#pragma once

#include <boost/asio.hpp>
#include <deque>
#include <string>
#include <thread>

class ClientApp {
public:
    ClientApp(const std::string& host, unsigned short port);
    void run();

private:
    void start_read();
    void handle_read(const boost::system::error_code& ec, std::size_t bytes);
    void send_line(std::string line);
    void do_write();
    void stop();

    boost::asio::io_context io_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::streambuf reply_buffer_;
    std::deque<std::string> outgoing_;
    bool write_in_progress_ = false;
    bool running_ = true;
    std::thread io_thread_;
};