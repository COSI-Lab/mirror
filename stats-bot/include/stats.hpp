#ifndef STATS_HPP
#define STATS_HPP

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <unordered_map>
#include <atomic>

namespace mirror::stats_bot
{

class StatsEntry
{
    public:
    StatsEntry(std::string name, long bytes_transferred)
    {
        this->name = name; this->bytes_transferred = bytes_transferred;
    }

    operator long() const { return bytes_transferred; }
    std::string name;
    long bytes_transferred;
};

std::vector<StatsEntry> get_stats();
nlohmann::json get_statistics_json();

size_t curl_write_callback(char *ch, size_t size, size_t nmeb, void* userdata);

extern std::unordered_map<int, std::string> stats_raw;
}

#endif