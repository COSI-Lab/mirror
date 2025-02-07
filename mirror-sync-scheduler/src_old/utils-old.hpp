#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <mirror/logger.hpp>
#include <nlohmann/json.hpp>

#include <mirror/sync_scheduler/queue.hpp>
#include <mirror/sync_scheduler/schedule.hpp>

namespace mirror::sync_scheduler
{
/**
 * @brief read json in from a file
 *
 * @param filename std::string json file to read
 *
 * @return json object
 *
 * @note
 * TODO: convert file parameter to `const std::filesystem::path&`
 */
auto read_JSON_from_file(const std::string& filename) -> nlohmann::json
{
    auto jsonObj = nlohmann::json::parse(std::ifstream(filename));

    return jsonObj;
}

/**
 * @brief thread for handling manual sync through stdin
 *
 * @note
 * !: I feel like there has to be a better way to do this
 *
 * TODO: Think of alternative methods for manual syncs. `stdin` could be useful
 * in addition to API endpoint, but probably not really needed.
 */
auto cin_thread() -> void
{
    // create a pointer to the queue
    auto* queue = Queue::getInstance();

    while (true)
    {
        std::string x;
        std::cin >> x;
        queue->manual_sync(x);
    }
}

/**
 * @brief thread that sends a message to the log server every 29 minutes to keep
 * the socket from closing
 *
 * @deprecated Log server may be going away soon
 *
 * @note
 * ?: Do we actually want to keep alive or let the connection drop and reconnect
 * when we need to send a message?
 */
auto keep_alive_thread() -> void
{
    auto* logger = mirror::Logger::getInstance();

    while (true)
    {
        // sleep for 29 minutes
        std::this_thread::sleep_for(std::chrono::minutes(29));
        // send keepalive message
        logger->info("keep alive.");
    }
}

/**
 * @brief thread that updates the schedule and syncCommandMap whenever there is
 * any change made to mirrors.json
 */
auto update_schedule_thread() -> void
{
    // create a pointer to the schedule
    auto* schedule = Schedule::getInstance();
    // create a pointer to the queue
    auto* queue    = Queue::getInstance();

    const auto mirrorsJSONFile
        = std::filesystem::relative("configs/mirrors.json");
    auto previousModificationTime
        = std::filesystem::last_write_time(mirrorsJSONFile);

    while (true)
    {
        // sleep for 7 seconds because its the smallest number that doesnt
        // divide 60
        // ?: why does the number have to be not divisible by 60?
        std::this_thread::sleep_for(std::chrono::seconds(7));

        const auto currentModificationTime
            = std::filesystem::last_write_time(mirrorsJSONFile);

        if (currentModificationTime != previousModificationTime)
        {
            // TODO: Use logger instead of `std::cout`
            std::cout << "`mirrors.json` last write time has changed. "
                         "Generating new sync schedule\n";

            // retrieve the mirror data from mirrors.json
            auto config = read_JSON_from_file("configs/mirrors.json");

            // build the schedule based on the mirrors.json config
            schedule->build(config.at("mirrors"));
            // create a new sync command map
            queue->createSyncCommandMap(config.at("mirrors"));
            // set reloaded flag for main thread
            schedule->reloaded = true;

            previousModificationTime = currentModificationTime;
        }
    }
}
} // namespace mirror::sync_scheduler
