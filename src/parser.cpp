#include "parser.h"

std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> parts;
    std::string cur;

    for (char c : line) {
        if (c == ' ') {
            if (!cur.empty()) {
                parts.push_back(cur);
                cur.clear();
            }
        } else {
            cur += c;
        }
    }

    if (!cur.empty()) {
        parts.push_back(cur);
    }

    return parts;
}
