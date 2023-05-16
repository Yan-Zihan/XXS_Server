#include "core/urlparser.h"

URLView::URLView(): _path(), _map(){}

std::string &URLView::path() {return _path;}

std::unordered_map<std::string, std::string> &URLView::params() {
    return _map;
}

URLView URLView::parse(std::string_view s) {
    URLView res;
    int pathend = -1;
    for (auto i = 1; i < s.length(); i++) {
        if (s[i] == '?'){
            pathend = i;
            break;
        }
    }
    if (pathend == -1) pathend = s.length();
    res._path = std::string(s.begin(), s.begin()+pathend);
    bool flag = 0;
    int ks = pathend+1, kt = -1, vs = -1, vt = -1;
    for (auto i = pathend+1; i <= s.length(); i++) {
        if (flag) {
            if (i == s.length() || s[i] == '&') {
                vt = i;
                if (kt-ks < 1) continue;
                res._map.insert_or_assign(std::string(s.begin()+ks, s.begin()+kt),\
                    std::string(s.begin()+vs, s.begin()+vt));
                flag = 0;
                ks = i+1;
            }
        } else {
            if (s[i] == '=') {
                kt = i;
                flag = 1;
                vs = i+1;
            }
        }
    }
    return res;
}