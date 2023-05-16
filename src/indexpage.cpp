#include "pages/indexpage.h"
#include <cppconn/exception.h>

extern sql::Connection *connection;
extern json config;
extern std::string serverUrl;

json::array_t getRandomGoods(int t) {
    json::array_t res;
    sql::Statement *stmt;
    try{
        stmt = connection->createStatement();
    } catch(sql::SQLException e) {
        connection->reconnect();
        connection->setSchema(config["globalConfig"]["database"]["schema"].get<std::string>());
        stmt = connection->createStatement();
    }
    auto queryResult = stmt->executeQuery(
        "SELECT GoodId, Name, Price FROM goods ORDER BY RAND() LIMIT " +
        std::to_string(t)
    );
    while (queryResult->next()) {
        auto id = util::encrypt(queryResult->getInt64("GoodId"));
        //auto id = queryResult->getInt64("GoodId");
        res.emplace_back(json::object({
            {"id", std::to_string(id)},
            {"price", queryResult->getInt("Price")},
            {"name", queryResult->getString("Name")},
            {"primaryPicUrl", serverUrl+"/res?dir=goods/"+std::to_string(id)+"&fname=index.jpg"}
        }));
    }
    delete queryResult;
    return res;
}

http::message_generator handleIndex(ReqInfo reqinfo, std::unordered_map<std::string, std::string> &&, std::string_view) {
    http::response<http::string_body> res(http::status::ok, 11);
    json j = {
        {"indexGoodsList", getRandomGoods(4)},
        {"banner", config["indexPage"]["banner"]},
        {"channel", config["indexPage"]["channel"]}
    };
    res.set(http::field::content_type, "application/json");
    res.keep_alive(reqinfo.first);
    res.body() = j.dump();
    res.prepare_payload();
    return res;
}