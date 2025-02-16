/**
 * @file main.cpp
 * @author Cary Keesler
 * @brief
 */

// Third Party Includes
#include <spdlog/cfg/env.h>

// Project Includes
#include <mirror/sync_scheduler/SyncScheduler.hpp>
#include <mirror/sync_scheduler/ManualSyncListener.hpp>

auto main() -> int
{
    spdlog::cfg::load_env_levels();

    mirror::sync_scheduler::SyncScheduler syncScheduler {};

    syncScheduler.run();

    return 0;
}
