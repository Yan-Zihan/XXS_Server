#ifndef URLPARSER_H
#define URLPARSER_H

#include <string>
#include <string_view>
#include <unordered_map>

class URLView {
    std::unordered_map<std::string, std::string> _map;
    std::string _path;
public:
    URLView();
    std::string &path();
    std::unordered_map<std::string, std::string> &params();
    static URLView parse(std::string_view);
};

#endif