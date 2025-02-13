/**
 * @file SyncScheduler.cpp
 * @author Cary Keesler
 * @brief
 */

// Header Being Defined
#include <mirror/sync_scheduler/SyncScheduler.hpp>

// Standard Library Includes
#include <fstream>
#include <stdexcept>

// Third Party Library Includes
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace mirror::sync_scheduler
{
SyncScheduler::SyncScheduler()
try
    : m_Schedule(load_config().value("mirrors", nlohmann::json {}))
{
    spdlog::info("Successfully generated schedule!");
}
catch (std::runtime_error& e)
{
    spdlog::error(e.what());
    throw e;
}

auto SyncScheduler::load_config() -> nlohmann::json
{
    std::ifstream configFile("configs/mirrors.json");

    return nlohmann::json::parse(configFile);
}

auto SyncScheduler::run() -> void { }
} // namespace mirror::sync_scheduler
