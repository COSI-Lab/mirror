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
    int                                                stdoutPipe = -1;
    int                                                stderrPipe = -1;
    std::chrono::time_point<std::chrono::system_clock> startTime;
};
} // namespace mirror::sync_scheduler
