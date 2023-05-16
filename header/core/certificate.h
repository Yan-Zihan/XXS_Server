#ifndef CERTIFICATE_H
#define CERTIFICATE_H

#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <cstddef>
#include <memory>

void load_server_certificate(boost::asio::ssl::context& ctx);

void load_root_certificates(boost::asio::ssl::context& ctx);

#endif