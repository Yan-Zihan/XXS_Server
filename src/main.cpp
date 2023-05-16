#include "core/listener.h"
#include "core/registerhandler.h"
#include "core/certificate.h"
#include <csignal>

extern util::TempFileCollector tempFileCollector;
request_session *requestSender;

static std::function<void()> _signal_handler;

static void signal_handler(int sig) {
    _signal_handler();
}

int main(int argc, char* argv[]) {
    if (argc != 3) return 0;
    auto const address = net::ip::make_address("0.0.0.0");
    auto const port = static_cast<unsigned short>(std::atoi(argv[1]));
    auto const threads = std::max<int>(2, std::atoi(argv[2]));
    try{
        initialize();
    } catch(sql::SQLException e) {
        std::cout<< e.getErrorCode()<<std::endl;
        return 0;
    }
    registerHandlers();
    // The io_context is required for all I/O
    net::io_context ioc{threads};
    ssl::context ctx_server{ssl::context::tlsv12};
    ssl::context ctx_client{ssl::context::tlsv12_client};
    load_server_certificate(ctx_server);
    load_root_certificates(ctx_client);
    ctx_client.set_verify_mode(ssl::verify_peer);
    requestSender = new request_session(&ioc, &ctx_client);
    // Create and launch a listening port
    std::make_shared<listener>(ioc, tcp::endpoint{address, port}, ctx_server) -> run();

    // Capture SIGINT and SIGTERM to perform a clean shutdown
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&](beast::error_code const&, int) {
        ioc.stop();
    });
    _signal_handler = [&]{
        logger->flush();
        spdlog::shutdown();
    };
    std::signal(SIGABRT, signal_handler);
    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back([&ioc] {
            ioc.run();
        });
    ioc.run();

    // (If we get here, it means we got a SIGINT or SIGTERM)

    // Block until all the threads exit
    tempFileCollector.stop();
    for(auto& t : v)
        t.join();
    logger->flush();
    return EXIT_SUCCESS;
}