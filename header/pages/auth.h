#ifndef AUTH_H
#define AUTH_H

#include "core/util.h"
http::message_generator handleLogin(ReqInfo, std::unordered_map<std::string, std::string> &&, std::string_view);
http::message_generator handleAuthCheck(ReqInfo, std::unordered_map<std::string, std::string> &&, std::string_view);
#endif