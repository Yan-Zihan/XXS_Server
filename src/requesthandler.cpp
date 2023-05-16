#include "core/requesthandler.h"

std::unordered_map<std::string,
    std::pair<typename RequestHandler::ResFuncType,http::verb>>
    RequestHandler::_map;

void RequestHandler::request(std::string_view req, ResFuncType &&func, http::verb method) {
    auto req_ = std::string(req.begin(), req.end());
    if (_map.find(req_) != _map.end()) return;
    _map.insert({req_, std::make_pair(func, method)});
}

auto RequestHandler::bad_request(std::string_view why, ReqType &req) -> ResType  {
    http::response<http::string_body> res{http::status::bad_request, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = std::string(why);
    res.prepare_payload();
    return res;
}

#include <fstream>
#include <cstdio>

auto RequestHandler::handleRequest(ReqType&& req) -> ResType {
    auto res = URLView::parse(std::string_view(req.target().data(), req.target().size()));
    auto func = _map.find(res.path());
    if (func == _map.end()) {
        //logger->error("wrong api: {}", res.path());
        return bad_request("wrong api", req);
    } else if (func->second.second != req.method()) {
        return bad_request("wrong method", req);
    } else {
        std::string_view body;
        auto it = req.find("content-type");
        if (it != req.end()
            && it->value().find("multipart/form-data;") != std::string::npos)
        {
            auto s = req.body().find("\r\n\r\n")+4;
            auto t = req.body().find("\r\n-----", s);
            body = std::string_view(req.body().data()+s, t-s);
        }
        else body = std::string_view(req.body().data(), req.body().size());
        return func->second.first(std::make_pair(req.keep_alive(), req.version()), \
            std::move(res.params()), body);
    }
}