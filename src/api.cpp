#include "core/registerhandler.h"
#include "core/util.h"
#include "pages/indexpage.h"
#include "pages/auth.h"
#include "pages/basicapi.h"
#include <fstream>

std::string serverUrl, homeDir;
static sql::Driver *driver = get_driver_instance();
sql::Connection *connection;
json config;
extern util::TempFileCollector tempFileCollector;

void initialize() {
    std::ifstream configJson("config.json");
    config = json::parse(configJson);
    serverUrl = config["globalConfig"]["serverUrl"].get<std::string>();
    homeDir = config["globalConfig"]["homeDirectory"].get<std::string>();
    try{
        auto dbconfig = config["globalConfig"]["database"];
        connection = driver->connect(dbconfig["host"].get<std::string>(),
            dbconfig["userName"].get<std::string>(),
            dbconfig["password"].get<std::string>()
        );
        connection->setSchema(dbconfig["schema"].get<std::string>());
    } catch(sql::SQLException e) {
        throw e;
    }
}

void registerHandlers() {
    tempFileCollector.run();
    RequestHandler::request("/", std::move(handleCheck), http::verb::get);

    RequestHandler::request("/res", std::move(handleFiles), http::verb::get);

    RequestHandler::request("/index/index", std::move(handleIndex), http::verb::get);

    RequestHandler::request("/upload", std::move(handleUploadFiles), http::verb::post);
    
    RequestHandler::request("/auth/loginByWeixin", std::move(handleLogin), http::verb::post);

    RequestHandler::request("/auth/check", std::move(handleAuthCheck), http::verb::get);
}