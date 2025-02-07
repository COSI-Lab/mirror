/**
 * @file SyncScheduler.cpp
 * @author Cary Keesler
 * @brief
 */

// Header Being Defined
#include <mirror/sync_scheduler/SyncScheduler.hpp>

// Standard Library Includes
#include <fstream>

// Third Party Library Includes
#include <nlohmann/json.hpp>

namespace mirror::sync_scheduler
{
SyncScheduler::SyncScheduler()
    : m_Schedule(load_config().value("mirrors", nlohmann::json {}))
{
}

auto SyncScheduler::load_config() -> nlohmann::json
{
    std::ifstream configFile("configs/mirrors.json");

    return nlohmann::json::parse(configFile);
}

auto SyncScheduler::run() -> void
{
    auto config = load_config();
    m_Schedule  = Schedule(config.at("mirrors"));
    m_Schedule.build();
}
} // namespace mirror::sync_scheduler
