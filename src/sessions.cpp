#include "core/sessions.h"

template<class Derived>
RequestHandler http_session<Derived>::handler_;

//-----------------------------------------------------------------------------------------

plain_http_session::plain_http_session(
    beast::tcp_stream&& stream,
    beast::flat_buffer&& buffer)
        : stream_(std::move(stream)),
          http_session<plain_http_session>(std::move(buffer)) {}

beast::tcp_stream& plain_http_session::stream() {
    return stream_;
}

void plain_http_session::run() {
    this->do_read();
}

void plain_http_session::do_eof() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
}

//-----------------------------------------------------------------------------------------

ssl_http_session::ssl_http_session(beast::tcp_stream&& stream,
    ssl::context& ctx,
    beast::flat_buffer&& buffer)
        : stream_(std::move(stream), ctx),
          http_session<ssl_http_session>(std::move(buffer)) {}

void ssl_http_session::run() {
    beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));
    // Note, this is the buffered version of the handshake.
    stream_.async_handshake(
        ssl::stream_base::server,
        buffer_.data(),
        beast::bind_front_handler(&ssl_http_session::on_handshake, shared_from_this())
    );
}

void ssl_http_session::do_eof() {
    // Set the timeout.
    beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));
    // Perform the SSL shutdown
    stream_.async_shutdown(
        beast::bind_front_handler(&ssl_http_session::on_shutdown, shared_from_this())
    );
}

void ssl_http_session::on_handshake(beast::error_code ec, std::size_t bytes_used) {
    if(ec){
        logger->error("handshake: {}", ec.message());
        return;
        //return fail(ec, "handshake");
    }
    else logger->info("handshake succeeded");
    // Consume the portion of the buffer used by the handshake
    buffer_.consume(bytes_used);
    do_read();
}

void ssl_http_session::on_shutdown(beast::error_code ec) {
    if(ec) logger->error("shutdown: {}", ec.message());
        //return fail(ec, "shutdown");
    // At this point the connection is closed gracefully
}

beast::ssl_stream<beast::tcp_stream>& ssl_http_session::stream() {
    return stream_;
}

//---------------------------------------------------------------------------------------

request_session::request_session(net::io_context *ioc, ssl::context *ctx) {
    ioc_ = ioc;
    ctx_ = ctx;
}

http::response<http::string_body> request_session::request(
    const std::string &host, const std::string target, const std::string port)
{
    beast::ssl_stream<beast::tcp_stream> stream_(*ioc_, *ctx_);
    tcp::resolver resolver_(*ioc_);
    beast::flat_buffer buffer_;
    if(!SSL_set_tlsext_host_name(stream_.native_handle(), host.data())) {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        throw beast::system_error{ec};
    }
    auto const results = resolver_.resolve(host, port);
    beast::get_lowest_layer(stream_).connect(results);
    stream_.handshake(ssl::stream_base::client);
    http::request<http::empty_body> req{http::verb::get, target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    http::write(stream_, req);
    http::response<http::string_body> res;
    http::read(stream_, buffer_, res);
    beast::error_code ec;
    stream_.shutdown(ec);
    return res;
}