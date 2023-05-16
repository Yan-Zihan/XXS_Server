#include "pages/auth.h"
#include "core/sessions.h"
#include "core/cipher.h"
#include <chrono>
#include <cppconn/exception.h>
#include <fmt/format.h>
#include <utility>

extern json config;

http::message_generator handleAuthCheck(
    ReqInfo info, std::unordered_map<std::string, std::string> &&map,
    std::string_view)
{
    auto userIdIt = map.find("id");
    auto tokenIt = map.find("token");
    if (userIdIt == map.end() || tokenIt == map.end())
        return error::bad_request("you bad bad", info);
    auto stmt = util::createStatement(connection);
    if (stmt == nullptr) return error::bad_request("mysql failed", info);
    auto queryString = fmt::format(
        "SELECT Token,NickName FROM users WHERE UserId={}",
        util::decrypt(std::atoll(userIdIt->second.c_str()))
    );
    auto queryRes = stmt->executeQuery(queryString);
    if (!queryRes->next()){
        http::response<http::string_body> res{http::status::ok, info.second};
        res.keep_alive(info.first);
        res.body() = "{\"errno\":3}";
        res.prepare_payload();
        return res;
    }
    json resJson = {
        {"errno", 0},
        {"nickName",queryRes->getString("NickName")},
        {"avatarUrl", serverUrl+"/res?dir=avatar&fname="+userIdIt->second+".jpg"}
    };
    http::response<http::string_body> res{http::status::ok, info.second};
    res.keep_alive(info.first);
    res.body() = resJson.dump();
    res.prepare_payload();
    delete queryRes;
    return res;
}

inline std::string createToken(std::string id, std::string key) {
    return cipher::encodeBase64(
        cipher::encryptAES(
            id,
            cipher::decodeBase64(key)
        )
    );
}

http::message_generator handleLogin(
    ReqInfo info, std::unordered_map<std::string, std::string> &&,
    std::string_view body)
{
    json j = json::parse(body);
    auto codeIt = j.find("code");
    auto avatarIt = j.find("avatarUrl");
    auto nameIt = j.find("nickName");
    if (codeIt == j.end() || avatarIt == j.end() || nameIt == j.end())
        return error::bad_request("keyword not found", info);
    const auto target = fmt::format(
        "/sns/jscode2session?appid={}&secret={}&js_code={}&grant_type=authorization_code",
        "wx5e991bc650dec431",
        "934070c61e8f52d504107351e40dc891",
        codeIt.value()
    );
    auto result = requestSender->request("api.weixin.qq.com", target, "443");
    json resj = json::parse(result.body());
    auto stmt = util::createStatement(connection);
    if (stmt == nullptr) return error::bad_request("mysql failed", info);
    auto queryStr = fmt::format(
        "INSERT INTO users \
        (OpenId,NickName,SessionKey,ExpireTime,Token) \
        VALUES({},{},{},{},{})",
        resj["openId"].get<std::string>(),
        nameIt.value(),
        resj["session_key"].get<std::string>(),
        fmt::format("{}", std::chrono::system_clock::now()+120h),
        createToken(resj["openId"].get<std::string>(),
            resj["session_key"].get<std::string>()
        )
    );
    try{
        stmt->executeUpdate(queryStr);
    } catch (sql::SQLException e) {
        return error::bad_request(e.what(), info);
    }
    http::response<http::string_body> res{http::status::ok, info.second};
    res.keep_alive(info.first);
    res.body() = "success";
    res.prepare_payload();
    return res;
}