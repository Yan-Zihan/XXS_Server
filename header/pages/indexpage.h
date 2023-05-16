#ifndef INDEXPAGE_H
#define INDEXPAGE_H

#include "core/util.h"
#include <filesystem>
#include <fstream>

extern std::string serverUrl, homeDir;

http::message_generator handleIndex(ReqInfo reqinfo, std::unordered_map<std::string, std::string> &&, std::string_view);

#endif