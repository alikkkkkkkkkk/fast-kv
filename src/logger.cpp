#include "logger.h"
#include <fstream>
#include <string>
#include <ctime>
#include <filesystem>

namespace {
    std::ofstream log_file;
    const std::size_t MAX_LOG_SIZE = 1024 * 1024;

    std::string timestamp() {
        std::time_t now = std::time(nullptr);
        std::tm* tm_now = std::localtime(&now);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_now);
        return std::string(buffer);
    }

    void reopen_if_needed(const std::string& filename) {
        namespace fs = std::filesystem;
        try {
            if (fs::exists(filename) && fs::file_size(filename) > MAX_LOG_SIZE) {
                log_file.close();
                log_file.open(filename, std::ios::trunc);
            }
        } catch (...) {
        }
    }
}

void log_init(const std::string& filename) {
    log_file.open(filename, std::ios::app);
}

void log_message(const std::string& msg) {
    if (!log_file.is_open()) {
        return;
    }
    const std::string filename = "fast-kv.log";
    reopen_if_needed(filename);
    log_file << "[" << timestamp() << "] " << msg << "\n";
    log_file.flush();
}
