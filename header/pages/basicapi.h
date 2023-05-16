#include "core/util.h"

http::message_generator handleFiles(
    ReqInfo, std::unordered_map<std::string, std::string> &&, std::string_view);

http::message_generator handleUploadFiles(
    ReqInfo, std::unordered_map<std::string, std::string> &&, std::string_view);

http::message_generator handleCheck(
    ReqInfo, std::unordered_map<std::string, std::string> &&, std::string_view);