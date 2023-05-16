#include "core/util.h"
#include <iomanip>

std::shared_ptr<spdlog::logger> logger =
    spdlog::daily_logger_mt<spdlog::async_factory>("daily_log", "logs/log.txt");

//-------------------------------------------------------------------------------------------

unsigned long long util::encrypt(long long x) {
    static int cipher[] = {2, 9, 6, 10, 14, 0, 1, 12, 11, 5, 3, 13, 4, 7, 15, 8};
    static int offset[] = {5, 5, -2, 7, 8, 4, -4, 6, 7, -8, -7, -3, -5, -2, -10, -1};
    auto i_offset = offset[x&15];
    unsigned long long r = 0;
    auto f = [](int n){
        return (n<16 && n>=0)? n: (n+16)%16;
    };
    for (int i=0; i<16; i++) {
        r |= (unsigned long long)cipher[f(((x&(15<<(i<<2)))>>(i<<2)) + i_offset + i)]<<(i<<2);
    }
    return r;
}

long long util::decrypt(unsigned long long x) {
    static int decipher[] = {5, 6, 0, 10, 12, 9, 2, 13, 15, 1, 3, 8, 7, 11, 4, 14};
    static int offset[] = {5, 5, -2, 7, 8, 4, -4, 6, 7, -8, -7, -3, -5, -2, -10, -1};
    int i_offset = offset[x&15];
    long long r = 0;
    auto f = [](int n){
        return (n<16 && n>=0)? n: (n+16)%16;
    };
    for (int i=0; i<16; i++) {
        r |= (long long)f(decipher[(x&(15<<(i<<2)))>>(i<<2)] - i_offset -i)<<(i<<2);
    }
    return r;
}

std::string util::snowflake() {
    char buf[27];
    static long long serial = 0;
    std::stringstream res;
    res << std::hex << std::this_thread::get_id();
    res << std::hex << serial++;
    return res.str();
}

extern json config;

sql::Statement *util::createStatement(sql::Connection *conn) {
    sql::Statement *res;
    try{
        res = conn->createStatement();
    } catch(sql::SQLException e) {
        if(conn->reconnect()){
            logger->info("mysql reconnected");
            conn->setSchema(
                config["globalConfig"]["database"]["schema"].get<std::string>()
            );
            res = conn->createStatement();
        } else {
            logger->error("mysql reconnection failed");
            return nullptr;
        }
    }
    return res;
}

//-------------------------------------------------------------------------------------------

util::TempFileCollector::TempFileCollector(): _tasks(), _taskLock(), isStarted(false) {}

void util::TempFileCollector::addTask(const TimePoint &t, std::string &file) {
    _taskLock.lock();
    _tasks.push(std::make_pair(t, file));
    _taskLock.unlock();
}

extern std::string homeDir;
#include <cstdio>

void util::TempFileCollector::run() {
    isStarted = true;
    std::thread([&]{
        std::queue<std::pair<TimePoint, std::string>> q;
        while(this->isStarted) {
            std::this_thread::sleep_for(1min);
            this->_taskLock.lock();
            if (this->_tasks.empty()) {
                this->_taskLock.unlock();
                continue;
            }
            auto now = std::chrono::system_clock::now();
            while (!this->_tasks.empty() && this->_tasks.front().first < now) {
                q.push(this->_tasks.front());
                this->_tasks.pop();
            }
            this->_taskLock.unlock();
            while (!q.empty()) {
                auto path = homeDir+"/res/tmp/"+q.front().second;
                std::remove(path.data());
                q.pop();
            }
        }
    }).detach();
}
void util::TempFileCollector::stop() {
    isStarted = false;
}

util::TempFileCollector tempFileCollector;

//-------------------------------------------------------------------------------------------

http::response<http::string_body>
    error::bad_request(std::string_view why, ReqInfo reqinfo) {
    http::response<http::string_body> res{http::status::bad_request, reqinfo.second};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(reqinfo.first);
    res.body() = std::string(why);
    res.prepare_payload();
    return res;
}

http::response<http::string_body>
    error::not_found(std::string_view target, ReqInfo reqinfo) {
    http::response<http::string_body> res{http::status::not_found, reqinfo.second};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(reqinfo.first);
    res.body() = "The resource '" + std::string(target) + "' was not found.";
    res.prepare_payload();
    return res;
}

http::response<http::string_body>
    error::server_error(std::string_view what, ReqInfo reqinfo) {
    http::response<http::string_body> res(http::status::internal_server_error, reqinfo.second);
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(reqinfo.first);
    res.body() = "An error occurred: '" + std::string(what) + "'";
    res.prepare_payload();
    return res;
}