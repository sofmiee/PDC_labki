#include "server.hpp"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <sstream>

using boost::asio::ip::tcp;
using error_code = boost::system::error_code;

namespace {
constexpr std::size_t kMaxLine = 4096;
constexpr int kTimeoutSeconds = 60;

std::string trim(const std::string& input) {
    std::size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start])) != 0) {
        ++start;
    }
    std::size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1])) != 0) {
        --end;
    }
    return input.substr(start, end - start);
}

std::string format_double(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value;
    return out.str();
}

bool is_integer(const std::string& text, int& value) {
    std::istringstream in(text);
    in >> value;
    return in && in.eof();
}

bool parse_calc(const std::string& line, double& left, char& op, double& right) {
    std::istringstream in(line);
    in >> left >> op >> right;
    return in && in.eof() && (op == '+' || op == '-' || op == '*' || op == '/');
}

std::optional<std::string> compute_average(const std::string& line) {
    std::istringstream in(line);
    std::vector<double> values;
    double value = 0.0;
    while (in >> value) {
        values.push_back(value);
    }
    if (!in.eof() || values.empty()) {
        return std::nullopt;
    }
    const double sum = std::accumulate(values.begin(), values.end(), 0.0);
    const double avg = sum / static_cast<double>(values.size());
    return "Average: " + format_double(avg);
}

std::string evaluate_expression(double left, char op, double right) {
    if (op == '+') return format_double(left + right);
    if (op == '-') return format_double(left - right);
    if (op == '*') return format_double(left * right);
    if (op == '/') {
        if (right == 0.0) {
            return "Error: division by zero";
        }
        return format_double(left / right);
    }
    return "Error: unknown operator";
}

unsigned long long factorial_checked(int n) {
    if (n < 0) {
        return 0;
    }
    if (n > 20) {
        return std::numeric_limits<unsigned long long>::max();
    }
    unsigned long long result = 1;
    for (int i = 2; i <= n; ++i) {
        result *= static_cast<unsigned long long>(i);
    }
    return result;
}
}

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, AsyncServer& server)
        : socket_(std::move(socket)),
          server_(server),
          session_strand_(server_.io_.get_executor()),
          timeout_timer_(socket_.get_executor()),
          write_in_progress_(false) {}

    void start() {
        boost::asio::post(
            boost::asio::bind_executor(session_strand_, [self = shared_from_this()]() {
                self->server_.active_connections_.fetch_add(1);
                self->server_.add_log_entry("Connected: " + self->remote_name());
                self->arm_timeout();
                self->do_read();
            }));
    }

private:
    void arm_timeout() {
        timeout_timer_.expires_after(std::chrono::seconds(kTimeoutSeconds));
        timeout_timer_.async_wait(boost::asio::bind_executor(
            session_strand_, [self = shared_from_this()](const error_code& ec) {
                if (!ec) {
                    self->enqueue_response("Error: session timeout");
                    self->close();
                }
            }));
    }

    void do_read() {
        if (!socket_.is_open()) {
            return;
        }
        boost::asio::async_read_until(
            socket_, input_buffer_, '\n',
            boost::asio::bind_executor(session_strand_,
                                       [self = shared_from_this()](const error_code& ec, std::size_t bytes) {
                                           self->on_read(ec, bytes);
                                       }));
    }

    void on_read(const error_code& ec, std::size_t bytes) {
        if (ec) {
            if (ec != boost::asio::error::operation_aborted) {
                server_.add_log_entry("Read error from " + remote_name() + ": " + ec.message());
            }
            close();
            return;
        }
        if (bytes > kMaxLine) {
            enqueue_response("Error: message too long");
            close();
            return;
        }
        timeout_timer_.cancel();
        std::istream in(&input_buffer_);
        std::string line;
        std::getline(in, line);
        handle_request(trim(line));
    }

    void handle_request(const std::string& request) {
        if (request.empty()) {
            enqueue_response("Error: empty request");
            arm_timeout();
            do_read();
            return;
        }
        if (request == "quit") {
            enqueue_response("Bye");
            close_after_write_ = true;
            return;
        }
        if (request.rfind("remind ", 0) == 0) {
            process_reminder(request);
            arm_timeout();
            do_read();
            return;
        }

        boost::asio::post(server_.io_, [self = shared_from_this(), request]() {
            std::string response = self->process_heavy_request(request);
            boost::asio::post(boost::asio::bind_executor(
                self->session_strand_, [self, response]() { self->enqueue_response(response); }));
        });

        arm_timeout();
        do_read();
    }

    void process_reminder(const std::string& request) {
        std::istringstream in(request);
        std::string cmd;
        int seconds = 0;
        in >> cmd >> seconds;
        std::string text;
        std::getline(in, text);
        text = trim(text);
        if (seconds < 0 || text.empty()) {
            enqueue_response("Error: use remind <seconds> <text>");
            return;
        }
        enqueue_response("Reminder scheduled in " + std::to_string(seconds) + " second(s)");

        auto timer = std::make_shared<boost::asio::steady_timer>(socket_.get_executor());
        timer->expires_after(std::chrono::seconds(seconds));
        timer->async_wait(boost::asio::bind_executor(
            session_strand_, [self = shared_from_this(), timer, text](const error_code& ec) {
                if (!ec) {
                    self->enqueue_response("Reminder: " + text);
                }
            }));
    }

    std::string process_heavy_request(const std::string& request) {
        double left = 0.0;
        double right = 0.0;
        char op = 0;
        if (parse_calc(request, left, op, right)) {
            const std::string answer = evaluate_expression(left, op, right);
            server_.add_log_entry("Calc [" + request + "] -> " + answer);
            return answer;
        }
        if (auto avg = compute_average(request); avg.has_value()) {
            server_.add_log_entry("Average [" + request + "] -> " + avg.value());
            return avg.value();
        }
        int n = 0;
        if (is_integer(request, n)) {
            const unsigned long long fact = factorial_checked(n);
            if (fact == std::numeric_limits<unsigned long long>::max()) {
                server_.add_log_entry("Factorial overflow for " + request);
                return "Error: factorial overflow for n > 20";
            }
            const std::string answer = "Factorial: " + std::to_string(fact);
            server_.add_log_entry("Factorial [" + request + "] -> " + answer);
            return answer;
        }
        server_.add_log_entry("Invalid request [" + request + "]");
        return "Error: unsupported request";
    }

    void enqueue_response(const std::string& text) {
        bool idle = outgoing_.empty();
        outgoing_.push_back(text + "\n");
        if (idle && !write_in_progress_) {
            do_write();
        }
    }

    void do_write() {
        if (outgoing_.empty() || !socket_.is_open()) {
            if (close_after_write_) {
                close();
            }
            return;
        }
        write_in_progress_ = true;
        boost::asio::async_write(
            socket_, boost::asio::buffer(outgoing_.front()),
            boost::asio::bind_executor(session_strand_,
                                       [self = shared_from_this()](const error_code& ec, std::size_t) {
                                           self->write_in_progress_ = false;
                                           if (ec) {
                                               self->server_.add_log_entry("Write error to " + self->remote_name() +
                                                                           ": " + ec.message());
                                               self->close();
                                               return;
                                           }
                                           self->outgoing_.erase(self->outgoing_.begin());
                                           if (!self->outgoing_.empty()) {
                                               self->do_write();
                                           } else if (self->close_after_write_) {
                                               self->close();
                                           }
                                       }));
    }

    std::string remote_name() const {
        error_code ec;
        auto endpoint = socket_.remote_endpoint(ec);
        if (ec) {
            return "unknown";
        }
        return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
    }

    void close() {
        if (!socket_.is_open()) {
            return;
        }
        timeout_timer_.cancel();
        error_code ignored;
        socket_.shutdown(tcp::socket::shutdown_both, ignored);
        socket_.close(ignored);
        server_.active_connections_.fetch_sub(1);
        server_.add_log_entry("Disconnected: " + remote_name());
    }

    tcp::socket socket_;
    AsyncServer& server_;
    boost::asio::strand<boost::asio::io_context::executor_type> session_strand_;
    boost::asio::streambuf input_buffer_;
    boost::asio::steady_timer timeout_timer_;
    std::vector<std::string> outgoing_;
    bool write_in_progress_;
    bool close_after_write_ = false;
};

AsyncServer::AsyncServer(boost::asio::io_context& io, unsigned short port, std::size_t thread_count)
    : io_(io),
      acceptor_(io_, tcp::endpoint(tcp::v4(), port)),
      strand_(io_.get_executor()),
      stopped_(false),
      thread_count_(thread_count == 0 ? 1 : thread_count),
      active_connections_(0) {}

void AsyncServer::run() {
    do_accept();
    workers_.reserve(thread_count_);
    for (std::size_t i = 0; i < thread_count_; ++i) {
        workers_.emplace_back([this]() {
            try {
                io_.run();
            } catch (const std::exception& ex) {
                add_log_entry(std::string("Worker exception: ") + ex.what());
            }
        });
    }
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void AsyncServer::stop() {
    if (stopped_.exchange(true)) {
        return;
    }
    error_code ignored;
    acceptor_.cancel(ignored);
    acceptor_.close(ignored);
    io_.stop();
}

void AsyncServer::do_accept() {
    acceptor_.async_accept([this](const error_code& ec, tcp::socket socket) {
        if (!ec) {
            std::make_shared<Session>(std::move(socket), *this)->start();
        } else if (!stopped_) {
            add_log_entry("Accept error: " + ec.message());
        }
        if (!stopped_) {
            do_accept();
        }
    });
}

void AsyncServer::add_log_entry(const std::string& entry) {
    boost::asio::post(strand_, [this, entry]() {
        log_storage_.push_back(entry);
        std::cout << entry << std::endl;
    });
}

SyncCalculatorServer::SyncCalculatorServer(boost::asio::io_context& io, unsigned short port)
    : acceptor_(io, tcp::endpoint(tcp::v4(), port)) {}

void SyncCalculatorServer::run() {
    for (;;) {
        tcp::socket socket(acceptor_.get_executor());
        acceptor_.accept(socket);
        try {
            for (;;) {
                boost::asio::streambuf buffer;
                boost::asio::read_until(socket, buffer, '\n');
                std::istream in(&buffer);
                std::string line;
                std::getline(in, line);
                line = trim(line);
                if (line == "quit") {
                    boost::asio::write(socket, boost::asio::buffer(std::string("Bye\n")));
                    break;
                }
                const std::string response = process_expression(line) + "\n";
                boost::asio::write(socket, boost::asio::buffer(response));
            }
        } catch (const std::exception&) {
        }
    }
}

std::string SyncCalculatorServer::process_expression(const std::string& line) {
    double left = 0.0;
    double right = 0.0;
    char op = 0;
    if (!parse_calc(line, left, op, right)) {
        return "Error: invalid expression";
    }
    return evaluate_expression(left, op, right);
}
