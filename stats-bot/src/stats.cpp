#include <stats.hpp>

#include <format>
#include <chrono>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <spdlog/spdlog.h>

constexpr std::string_view PROMETHEUS_FORMAT = "http://mirror-prometheus:9090/api/v1/query_range?query=bytes_sent&start={}&end={}&step=24h";
constexpr std::string_view DATESTRING_FORMAT = "{0:%F}T{0:%T}Z";

std::vector<mirror::stats_bot::StatsEntry> mirror::stats_bot::get_stats()
{
    nlohmann::json stats_json = get_statistics_json();
    std::vector<mirror::stats_bot::StatsEntry> stats_data{};


    int num_entries = stats_json["data"]["result"].size();
    for(int i = 0; i < num_entries; i++)
    {
        nlohmann::json entry = stats_json["data"]["result"][i];
        mirror::stats_bot::StatsEntry{entry["metric"]["project"], std::stol(entry["values"][1].dump())};
    }

    return stats_data;
}

std::atomic_int counter = 0;

/**
 * Fetches statistics on bytes transferred over the last 24 hours.
 * This call will block.
 */
nlohmann::json mirror::stats_bot::get_statistics_json()
{
    CURL *curl = curl_easy_init();
    if(!curl)
    {
        spdlog::error("cURL failed to initialize");
        return nlohmann::json{};
    }

    std::chrono::time_point now{std::chrono::system_clock::now()};
    std::string url = std::format(PROMETHEUS_FORMAT, 
        std::format(DATESTRING_FORMAT, now),
        std::format(DATESTRING_FORMAT, now - std::chrono::days{1}));

    int *req_id = new int;
    *req_id = counter++;

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, req_id);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    
    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
        spdlog::error("cURL execution failed");
    }

    curl_easy_cleanup(curl);

    nlohmann::json data{nlohmann::json::parse(stats_raw[*req_id])};
    stats_raw.erase(*req_id);
    delete req_id;
    return data;
}

std::unordered_map<int, std::string> mirror::stats_bot::stats_raw{};
size_t mirror::stats_bot::curl_write_callback(char *ch, size_t size, size_t nmeb, void* userdata)
{
    int id = *(int*)userdata;
    if(!stats_raw.contains(id)) stats_raw[id] = std::string{};
    stats_raw[id] += ch;
    return size * nmeb;
}
