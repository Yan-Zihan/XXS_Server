#ifndef SESSION_H
#define SESSION_H

#include "util.h"
#include "requesthandler.h"
#include <optional>

template<class Derived>
class http_session {
    Derived& derived() {
        return static_cast<Derived&>(*this);
    }

    static constexpr std::size_t queue_limit = 8; // max responses
    std::vector<http::message_generator> response_queue_;
    static RequestHandler handler_;
    // The parser is stored in an optional container so we can
    // construct it from scratch it at the beginning of each new message.
    boost::optional<http::request_parser<http::string_body>> parser_;
protected:
    beast::flat_buffer buffer_;
public:
    // Construct the session
    http_session(beast::flat_buffer buffer)
        : buffer_(std::move(buffer)) {}

    void do_read() {
        parser_.emplace();
        parser_->body_limit(10000000);

        beast::get_lowest_layer(
            derived().stream()).expires_after(std::chrono::seconds(30));

        http::async_read(
            derived().stream(), buffer_, *parser_,
            beast::bind_front_handler(
                &http_session::on_read,
                derived().shared_from_this()
            )
        );
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if(ec == http::error::end_of_stream)
            return derived().do_eof();

        if(ec) {
            logger->error("read: {}", ec.message());
            return;
        }

        /* See if it is a WebSocket Upgrade
        if(websocket::is_upgrade(parser_->get())) {
            // Disable the timeout.
            // The websocket::stream uses its own timeout settings.
            beast::get_lowest_layer(derived().stream()).expires_never();

            // Create a websocket session, transferring ownership
            // of both the socket and the HTTP request.
            return make_websocket_session(
                derived().release_stream(),
                parser_->release());
        }*/

        // Send the response
        auto token = parser_->get()["Athorization"];
        queue_write(handler_.handleRequest(parser_->release()));

        // If we aren't at the queue limit, try to pipeline another request
        if (response_queue_.size() < queue_limit)
            do_read();
    }

    void queue_write(http::message_generator response) {
        // Allocate and store the work
        response_queue_.push_back(std::move(response));
        if (response_queue_.size() == 1)
            do_write();
    }

    // Called to start/continue the write-loop. Should not be called when
    // write_loop is already active.
    //
    // Returns `true` if the caller may initiate a new read
    bool do_write() {
        bool const was_full =
            response_queue_.size() == queue_limit;

        if(! response_queue_.empty()) {
            http::message_generator msg =
                std::move(response_queue_.front());
            response_queue_.erase(response_queue_.begin());

            bool keep_alive = msg.keep_alive();

            beast::async_write(
                derived().stream(), std::move(msg),
                beast::bind_front_handler(
                    &http_session::on_write,
                    derived().shared_from_this(),
                    keep_alive
                )
            );
        }
        return was_full;
    }

    void on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if(ec) {
            logger->error("write: {}", ec.message());
            return;
        }

        if(! keep_alive) {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return derived().do_eof();
        }

        // Inform the queue that a write completed
        if(do_write()) {
            // Read another request
            do_read();
        }
    }
};

class plain_http_session
    : public std::enable_shared_from_this<plain_http_session>,
      public http_session<plain_http_session> {
    beast::tcp_stream stream_;

public:
    // Create the http_session
    plain_http_session(
        beast::tcp_stream&& stream,
        beast::flat_buffer&& buffer);
    void run();
    void do_eof();
    beast::tcp_stream& stream();
};

class ssl_http_session
    : public std::enable_shared_from_this<ssl_http_session>,
      public http_session<ssl_http_session> {
    beast::ssl_stream<beast::tcp_stream> stream_;

public:
    // Create the http_session
    ssl_http_session(beast::tcp_stream&& stream,
        ssl::context& ctx,
        beast::flat_buffer&& buffer);
    void run();
    void do_eof();
    beast::ssl_stream<beast::tcp_stream>& stream();
private:
    void on_handshake(beast::error_code ec, std::size_t bytes_used);
    void on_shutdown(beast::error_code ec);

};

class request_session {
    net::io_context *ioc_;
    ssl::context *ctx_;
public:
    request_session(net::io_context *ioc, ssl::context *ctx);
    http::response<http::string_body> request(
        const std::string &host, const std::string target, const std::string port);
};

extern request_session *requestSender;

#endif