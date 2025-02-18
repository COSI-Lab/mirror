/**
 * @file main.cpp
 * @author Cary Keesler
 * @brief
 */

// Standard Library Includes
#include <exception>

// Third Party Includes
#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>

// Project Includes
#include <mirror/sync_scheduler/SyncScheduler.hpp>
#include <mirror/sync_scheduler/ManualSyncListener.hpp>

auto main() -> int
{
    spdlog::cfg::load_env_levels();

    try
    {
        mirror::sync_scheduler::SyncScheduler syncScheduler {};
        syncScheduler.run();
    }
    catch (std::exception& e)
    {
        spdlog::error("{}", e.what());
        return 0;
    }
}
