#ifndef UTIL_H
#define UTIL_H
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <memory>
#include <functional>
#include <thread>
#include <chrono>
#include <ctime>
#include <queue>
#include <mutex>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <spdlog/async.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = boost::asio::ip::tcp;
using ReqInfo = std::pair<bool, unsigned>;
using json = nlohmann::json;
namespace net = boost::asio;
namespace http = boost::beast::http;
namespace ssl = boost::asio::ssl;
using namespace std::chrono_literals;

extern std::shared_ptr<spdlog::logger> logger;
extern sql::Connection *connection;
extern std::string serverUrl, homeDir;

namespace util{
    unsigned long long encrypt(long long x);
    long long decrypt(unsigned long long x);
    std::string snowflake();

    class TempFileCollector {
    private:
        using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
        using T = std::pair<TimePoint, std::string>;
        std::queue<T> _tasks;
        std::mutex _taskLock;
        bool isStarted;
    public:

        TempFileCollector();
        void addTask(const TimePoint &t, std::string &file);
        void run();
        void stop();
    };

    sql::Statement *createStatement(sql::Connection *conn);
}

namespace error{
using ReqInfo = std::pair<bool, unsigned>;
http::response<http::string_body>
    bad_request(std::string_view why, ReqInfo reqinfo);

http::response<http::string_body>
    not_found(std::string_view target, ReqInfo reqinfo);

http::response<http::string_body>
    server_error(std::string_view what, ReqInfo reqinfo);
}

#endif