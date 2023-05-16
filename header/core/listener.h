#ifndef LISTENER_H
#define LISTENER_H

#include "sessions.h"

class detect_session : public std::enable_shared_from_this<detect_session> {
    beast::tcp_stream stream_;
    ssl::context& ctx_;
    beast::flat_buffer buffer_;

public:
    explicit
    detect_session(tcp::socket&& socket, ssl::context& ctx);

    void run();

    void on_run();

    void on_detect(beast::error_code ec, bool result);
};

class listener : public std::enable_shared_from_this<listener> {
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    ssl::context &ctx_;

public:
    listener(net::io_context& ioc, tcp::endpoint endpoint, ssl::context &ctx);

    void run();

private:
    void do_accept();

    void on_accept(beast::error_code ec, tcp::socket socket);
};

#endif