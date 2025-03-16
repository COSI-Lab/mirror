/**
 * @file JobManager.hpp
 * @author Cary Keesler
 * @brief
 */

#pragma once

// System Includes
#include <sys/types.h>

// Standard Library Includes
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Project Includes
#include <mirror/sync_scheduler/SyncJob.hpp>

namespace mirror::sync_scheduler
{
class JobManager
{
  public: // Constructors
    JobManager();
    JobManager(JobManager&) = delete;
    JobManager(JobManager&&) = delete;
    auto operator=(JobManager&) -> JobManager = delete;
    auto operator=(JobManager&&) -> JobManager = delete;

    ~JobManager();

  public: // Methods
    auto start_job(
        const std::string&           jobName,
        std::string                  command,
        const std::filesystem::path& passwordFile
    ) -> bool;

  private: // Methods
    auto job_is_running(const std::string& jobName) -> bool;
    auto register_job(
        const std::string& jobName,
        const ::pid_t      processID,
        const int          stdoutPipe,
        const int          stderrPipe
    ) -> void;
    auto kill_all_jobs() -> void;
    auto reap_processes() -> std::vector<::pid_t>;
    auto deregister_jobs(const std::vector<::pid_t>& completedJobs) -> void;

  private: // Static Methods
    static auto get_child_process_ids(const ::pid_t processID)
        -> std::vector<::pid_t>;
    static auto interrupt_job(const ::pid_t processID) -> bool;
    static auto kill_job(const ::pid_t processID) -> void;

  private: // Members
    std::map<::pid_t, SyncJob> m_ActiveJobs;
    std::jthread               m_ProcessReaper;
    std::condition_variable    m_SleepVariable;
    std::mutex                 m_JobMutex;
    std::mutex                 m_ReaperMutex;
};
} // namespace mirror::sync_scheduler
