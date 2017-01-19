#include "stl.h"

std::vector<std::string> stl::splitMsg(std::string msg) {
    std::string next;
    std::vector<std::string> result;

    for (std::string::const_iterator it = msg.begin(); it != msg.end(); it++) {
        if (*it != MSG_DELIMITER) {
            next += *it;
        } else {
            if (!next.empty()) {
                result.push_back(next);
                next.clear();
            }
        }
    }
    if (!next.empty())
        result.push_back(next);
    return result;
}