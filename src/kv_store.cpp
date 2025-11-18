#include "kv_store.h"
#include "parser.h"
#include "stats.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>

static std::unordered_map<std::string, std::string> storage;
static std::mutex storage_mutex;

std::string handle_command(const std::string& line) {
    auto parts = split(line);
    if (parts.empty()) {
        stats_inc_error();
        return "ERROR empty command\n";
    }

    stats_inc_command();

    const std::string& cmd = parts[0];

    if (cmd == "SET") {
        if (parts.size() < 3) {
            stats_inc_error();
            return "ERROR usage: SET key value\n";
        }

        const std::string& key = parts[1];
        std::string value;

        for (size_t i = 2; i < parts.size(); ++i) {
            if (i > 2) value += ' ';
            value += parts[i];
        }

        std::lock_guard<std::mutex> lock(storage_mutex);
        storage[key] = value;
        return "OK\n";
    }

    if (cmd == "GET") {
        if (parts.size() != 2) {
            stats_inc_error();
            return "ERROR usage: GET key\n";
        }

        const std::string& key = parts[1];

        std::lock_guard<std::mutex> lock(storage_mutex);
        auto it = storage.find(key);
        if (it == storage.end()) return "NOT_FOUND\n";
        return "VALUE " + it->second + "\n";
    }

    if (cmd == "DEL") {
        if (parts.size() != 2) {
            stats_inc_error();
            return "ERROR usage: DEL key\n";
        }

        const std::string& key = parts[1];

        std::lock_guard<std::mutex> lock(storage_mutex);
        auto it = storage.find(key);
        if (it == storage.end()) return "NOT_FOUND\n";
        storage.erase(it);
        return "DELETED\n";
    }

    if (cmd == "EXISTS") {
        if (parts.size() != 2) {
            stats_inc_error();
            return "ERROR usage: EXISTS key\n";
        }

        const std::string& key = parts[1];

        std::lock_guard<std::mutex> lock(storage_mutex);
        return storage.count(key) ? "1\n" : "0\n";
    }

    if (cmd == "COUNT") {
        if (parts.size() != 1) {
            stats_inc_error();
            return "ERROR usage: COUNT\n";
        }

        std::lock_guard<std::mutex> lock(storage_mutex);
        return std::to_string(storage.size()) + "\n";
    }

    if (cmd == "KEYS") {
        if (parts.size() != 1) {
            stats_inc_error();
            return "ERROR usage: KEYS\n";
        }

        std::lock_guard<std::mutex> lock(storage_mutex);
        if (storage.empty()) return "EMPTY\n";

        std::string result = "KEYS";
        for (const auto& kv : storage) result += " " + kv.first;
        result += "\n";
        return result;
    }

    if (cmd == "CLEAR") {
        if (parts.size() != 1) {
            stats_inc_error();
            return "ERROR usage: CLEAR\n";
        }

        std::lock_guard<std::mutex> lock(storage_mutex);
        storage.clear();
        return "CLEARED\n";
    }

    if (cmd == "INCR") {
        if (parts.size() != 2) {
            stats_inc_error();
            return "ERROR usage: INCR key\n";
        }
        const std::string& key = parts[1];

        std::lock_guard<std::mutex> lock(storage_mutex);

        long long value = 0;
        auto it = storage.find(key);

        if (it != storage.end()) {
            try {
                value = std::stoll(it->second);
            } catch (...) {
                stats_inc_error();
                return "ERROR value is not integer\n";
            }
        }

        ++value;
        storage[key] = std::to_string(value);
        return "VALUE " + std::to_string(value) + "\n";
    }

    if (cmd == "STATS") {
        if (parts.size() != 1) {
            stats_inc_error();
            return "ERROR usage: STATS\n";
        }
        return stats_render();
    }

    stats_inc_error();
    return "ERROR unknown command\n";
}
