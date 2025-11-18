#include "stats.h"
#include <atomic>
#include <string>

static std::atomic<long long> connections_now{0};
static std::atomic<long long> connections_total{0};
static std::atomic<long long> commands_total{0};
static std::atomic<long long> errors_total{0};

void stats_inc_connection() {
    ++connections_now;
    ++connections_total;
}

void stats_dec_connection() {
    --connections_now;
}

void stats_inc_command() {
    ++commands_total;
}

void stats_inc_error() {
    ++errors_total;
}

std::string stats_render() {
    return "STATS connections_now=" + std::to_string(connections_now.load()) +
           " connections_total=" + std::to_string(connections_total.load()) +
           " commands_total=" + std::to_string(commands_total.load()) +
           " errors_total=" + std::to_string(errors_total.load()) + "\n";
}
