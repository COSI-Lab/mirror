/**
 * @file SyncJob.hpp
 * @author Cary Keesler
 * @brief
 */

#pragma once

// System Includes
#include <unistd.h>

// Standard Library Includes
#include <chrono>

namespace mirror::sync_scheduler
{
struct SyncJob
{
    ::pid_t                                            processID  = 0;
    int                                                stdoutPipe = -1;
    int                                                stderrPipe = -1;
    std::chrono::time_point<std::chrono::system_clock> startTime;
};
} // namespace mirror::sync_scheduler
