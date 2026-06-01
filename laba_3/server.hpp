#pragma once

#include <boost/asio.hpp>
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class Session;

class AsyncServer {
public:
    AsyncServer(boost::asio::io_context& io, unsigned short port, std::size_t thread_count);
    void run();
    void stop();

private:
    void do_accept();
    void add_log_entry(const std::string& entry);

    boost::asio::io_context& io_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    std::vector<std::string> log_storage_;
    std::vector<std::thread> workers_;
    std::atomic<bool> stopped_;
    std::size_t thread_count_;
    std::atomic<std::size_t> active_connections_;

    friend class Session;
};

class SyncCalculatorServer {
public:
    SyncCalculatorServer(boost::asio::io_context& io, unsigned short port);
    void run();

private:
    static std::string process_expression(const std::string& line);

    boost::asio::ip::tcp::acceptor acceptor_;
};
