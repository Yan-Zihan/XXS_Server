#include "core/listener.h"

listener::listener(net::io_context& ioc, tcp::endpoint endpoint, ssl::context &ctx)
    : ioc_(ioc)
    , acceptor_(net::make_strand(ioc))
    , ctx_(ctx) {
    beast::error_code ec;
    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if(ec) {
        logger->error("open: {}", ec.message());
        //fail(ec, "open");
        return;
    }
    // Allow address reuse
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if(ec) {
        logger->error("set_option: {}", ec.message());
        //fail(ec, "set_option");
        return;
    }
    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if(ec) {
        logger->error("bind: {}", ec.message());
        //fail(ec, "bind");
        return;
    }
    // Start listening for connections
    acceptor_.listen(
        net::socket_base::max_listen_connections, ec);
    if(ec) {
        logger->error("listen: {}", ec.message());
        //fail(ec, "listen");
        return;
    }
}

void listener::run() {
    net::dispatch(acceptor_.get_executor(),
        beast::bind_front_handler(&listener::do_accept, this->shared_from_this())
    );
}

void listener::do_accept() {
    acceptor_.async_accept(net::make_strand(ioc_),
        beast::bind_front_handler(&listener::on_accept, shared_from_this())
    );
}

void listener::on_accept(beast::error_code ec, tcp::socket socket) {
    if(ec) logger->error("accept: {}", ec.message());
    //fail(ec, "accept");
    else {
        // Create the http session and run it
        std::make_shared<detect_session>(std::move(socket), ctx_) -> run();
    }
    // Accept another connection
    do_accept();
}

detect_session::detect_session(tcp::socket&& socket, ssl::context& ctx)
    : stream_(std::move(socket)), ctx_(ctx) {}

void detect_session::run() {
    net::dispatch(stream_.get_executor(),
        beast::bind_front_handler(&detect_session::on_run,this->shared_from_this())
    );
}

void detect_session::on_run() {
    stream_.expires_after(std::chrono::seconds(30));
    beast::async_detect_ssl(
        stream_, buffer_,
        beast::bind_front_handler(&detect_session::on_detect, this->shared_from_this())
    );
}

void detect_session::on_detect(beast::error_code ec, bool result) {
    if(ec) {
        logger->error("detect: {}", ec.message());
        return;
    }
        //return fail(ec, "detect");
    if(result) {
        // Launch SSL session
        std::make_shared<ssl_http_session>(std::move(stream_), ctx_, std::move(buffer_))->run();
        return;
    }
    // Launch plain session
    std::make_shared<plain_http_session>(std::move(stream_), std::move(buffer_))->run();
}