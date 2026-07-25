/**
 * @file SyncJob.hpp
 * @author Cary Keesler
 * @brief
 */

#pragma once

// Standard Library Includes
#include <chrono>
#include <string>

namespace mirror::sync_scheduler
{
struct SyncJob
{
    std::string                                        jobName    = "";
    // merged stdout and stderr
    int                                                stdPipe = -1;
    std::chrono::time_point<std::chrono::system_clock> startTime;
    bool isRsync = false;
};
} // namespace mirror::sync_scheduler
