#pragma once
#include <regex>
#include <string>

namespace policy {
inline bool validName(const std::string &name) {
    static const std::regex pattern("ugm_[a-z][a-z0-9_]{0,26}");
    return std::regex_match(name, pattern);
}
inline bool canManage(const std::string &name, unsigned uid, const std::string &operatorName) {
    return validName(name) && uid >= 1000 && uid != 65534 && name != operatorName;
}
inline bool confirmDelete(const std::string &name, const std::string &text) {
    return validName(name) && text == "DELETE " + name;
}
inline bool hasPassword(std::string value) {
    const auto first = value.find_first_not_of('!');
    return first != std::string::npos && value[first] != '*';
}
}
