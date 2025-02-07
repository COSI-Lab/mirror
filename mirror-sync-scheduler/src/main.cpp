/**
 * @file main.cpp
 * @author Cary Keesler
 * @brief
 */

// Project Includes
#include <mirror/sync_scheduler/SyncScheduler.hpp>

auto main() -> int
{
    mirror::sync_scheduler::SyncScheduler syncScheduler {};

    syncScheduler.run();

    return 0;
}
