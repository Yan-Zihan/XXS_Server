#include "pages/basicapi.h"
#include <fstream>

extern util::TempFileCollector tempFileCollector;

http::message_generator handleUploadFiles(
    ReqInfo reqinfo,
    std::unordered_map<std::string, std::string> &&map,
    std::string_view reqBody)
{
    std::string filePath = homeDir+"/res/tmp/";
    std::string fileName = util::snowflake()+".jpg";
    std::ofstream fout(filePath+fileName, std::ios_base::binary|std::ios_base::out);
    http::response<http::string_body> res{http::status::ok, reqinfo.second};
    res.keep_alive(reqinfo.first);
    fout.write(reqBody.data(), reqBody.size());
    fout.close();
    tempFileCollector.addTask(std::chrono::system_clock::now()+5min, fileName);
    res.set(http::field::content_type,"application/json");
    //res.set(http::field::access_control_allow_origin, "*");
    json j = {
        {"code", "success"},
        {"url", serverUrl+"/res?dir=tmp&fname="+fileName}
    };
    std::cout<<j.dump();
    res.body() = j.dump();
    res.prepare_payload();
    return res;
}

http::message_generator handleFiles(
    ReqInfo reqinfo,
    std::unordered_map<std::string, std::string> &&map,
    std::string_view reqBody)
{
    auto fileNameIt = map.find("fname");
    auto dirNameIt = map.find("dir");
    if (fileNameIt == map.end() || dirNameIt == map.end()) {
        return error::bad_request("wrong query", reqinfo);
    }
    if (dirNameIt->second.find('.') != std::string::npos ||
        fileNameIt->second.find('/') != std::string::npos) {
        return error::bad_request("naughty", reqinfo);
    }
    http::file_body::value_type body;
    beast::error_code ec;
    auto path =homeDir+"/res/"+
        dirNameIt->second+"/"+fileNameIt->second;
    body.open(path.c_str(), beast::file_mode::scan,ec);
    auto size = body.size();
    if(ec == beast::errc::no_such_file_or_directory)
        return error::not_found(fileNameIt->second, reqinfo);
    if(ec)
        return error::server_error(ec.message(), reqinfo);
    http::response<http::file_body> res(std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, reqinfo.second));
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "image/jpeg");
    res.content_length(size);
    res.keep_alive(reqinfo.first);
    return res;
}

http::message_generator handleCheck(
    ReqInfo reqinfo,
    std::unordered_map<std::string, std::string> &&map,
    std::string_view reqBody)
{
    http::response<http::string_body> res{http::status::ok, reqinfo.second};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(reqinfo.first);
    res.body()="work work";
    res.prepare_payload();
    return res;
}