/**
 * @file JobManager.cpp
 * @author Cary Keesler
 * @brief
 */

// Header Being Defined
#include <mirror/sync_scheduler/JobManager.hpp>

// System Includes
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Standard Library Includes
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <ranges>
#include <stop_token>
#include <string>
// NOLINTNEXTLINE(*-deprecated-headers, llvm-include-order)
#include <string.h> // Required for `::strerror_r`. Not available in `cstring`
#include <thread>
#include <vector>

// Third Party Includes
#include <spdlog/spdlog.h>

// Project Includes
#include <mirror/sync_scheduler/SyncJob.hpp>

namespace mirror::sync_scheduler
{
JobManager::JobManager()
{
    m_ProcessReaper = std::jthread(
        [this](const std::stop_token& stopToken) -> void
        {
            while (true)
            {
                spdlog::trace("Process reaper thread going to sleep");
                std::unique_lock<std::mutex> reaperLock(m_ReaperMutex);
                m_SleepVariable.wait_for(reaperLock, std::chrono::minutes(1));
                spdlog::trace("Process reaper thread woke up");

                if (stopToken.stop_requested())
                {
                    spdlog::info("Process reaper thread stop requested");
                    kill_all_jobs();

                    return;
                }

                auto completedJobs = reap_processes();

                deregister_jobs(completedJobs);
                completedJobs.clear();
            }
        }
    );
}

JobManager::~JobManager()
{
    spdlog::info("Joining process reaper thread");
    m_ProcessReaper.request_stop();
    m_SleepVariable.notify_all();
    m_ProcessReaper.join();
    spdlog::info("Process reaper thread joined!");
}

auto JobManager::reap_processes() -> std::vector<::pid_t>
{
    static std::string errorMessage(BUFSIZ, '\0');
    errorMessage.clear();

    static const ::pid_t syncSchedulerProcessID = ::getpid();

    static std::vector<::pid_t> completedJobs;
    completedJobs.clear();
    completedJobs.reserve(m_ActiveJobs.size());

    const std::lock_guard<std::mutex> JobLock(m_JobMutex);
    std::ifstream                     childrenFile(
        std::format("/proc/{0}/tasks/{0}/children", syncSchedulerProcessID)
    );

    for (const ::pid_t childProcessID :
         std::views::istream<::pid_t>(childrenFile))
    {
        spdlog::trace(
            "Attempting to wait on process with pid {}",
            childProcessID
        );

        int status = 0;

        // NOLINTBEGIN(misc-include-cleaner)
        const ::pid_t waitReturn = ::waitpid(childProcessID, &status, WNOHANG);
        // NOLINTEND(misc-include-cleaner)

        const bool isKnownJob = m_ActiveJobs.contains(childProcessID);

        if (waitReturn == 0) // Process still running
        {
            if (!isKnownJob)
            {
                continue;
            }

            constexpr std::uint8_t hoursBeforeTimeout = 6;
            const auto syncDuration = std::chrono::system_clock::now()
                                    - m_ActiveJobs.at(childProcessID).startTime;
            if (syncDuration < std::chrono::hours(hoursBeforeTimeout))
            {
                continue;
            }

            spdlog::warn(
                "Project {} has been syncing for at least {} hour{}. Process "
                "may be hanging, killing process. (pid: {})",
                m_ActiveJobs.at(childProcessID).jobName,
                hoursBeforeTimeout,
                (hoursBeforeTimeout == 1 ? "" : "s"), // if not one hour, plural
                childProcessID
            );

            kill_job(childProcessID);

            completedJobs.emplace_back(childProcessID);
            continue;
        }
        else if (waitReturn == -1) // waitpid() failed
        {
            spdlog::error(
                "waitpid() returned -1 for process with pid: {}! Error "
                "message: {}",
                ::strerror_r(errno, errorMessage.data(), errorMessage.size()),
                childProcessID
            );
            completedJobs.emplace_back(childProcessID);
            continue;
        }

        const int exitStatus = WEXITSTATUS(status);

        if (exitStatus == EXIT_SUCCESS)
        {
            if (isKnownJob)
            {
                spdlog::info(
                    "Project {} successfully synced! (pid: {})",
                    m_ActiveJobs.at(childProcessID).jobName,
                    childProcessID
                );
            }
            else
            {
                spdlog::info(
                    "Reaped successful unregistered child process with pid {}",
                    childProcessID
                );
            }
        }
        else
        {
            if (isKnownJob)
            {
                spdlog::warn(
                    "Project {} failed to sync! Exit code: {} (pid: {})",
                    m_ActiveJobs.at(childProcessID).jobName,
                    exitStatus,
                    childProcessID
                );
            }
            else
            {
                spdlog::warn(
                    "Reaped unsuccessful unregistered child process with pid "
                    "{}",
                    childProcessID
                );
            }
        }

        completedJobs.emplace_back(childProcessID);
    }

    return completedJobs;
}

auto JobManager::job_is_running(const std::string& jobName) -> bool
{
    return std::ranges::any_of(
        m_ActiveJobs,
        [&jobName](const auto& job) -> bool
        { return job.second.jobName == jobName; }
    );
}

auto JobManager::kill_job(const ::pid_t processID) -> void
{
    // NOLINTNEXTLINE(misc-include-cleaner)
    const int killReturn = ::kill(processID, SIGKILL);

    if (killReturn == 0)
    {
        spdlog::trace("Successfully sent process {} a SIGKILL", processID);
    }
    else
    {
        static std::string errorMessage(BUFSIZ, '\0');
        errorMessage.clear();

        spdlog::error(
            "Failed to send process {} a SIGKILL! Error message: {}",
            processID,
            processID,
            ::strerror_r(errno, errorMessage.data(), errorMessage.size())
        );
    }

    const int waitReturn = ::waitpid(processID, nullptr, 0);

    if (waitReturn == processID)
    {
        spdlog::trace("Process {} successfully reaped", processID);
    }
    else
    {
        spdlog::error("Failed to reap process {} after SIGKILL!", processID);
    }
}

auto JobManager::kill_all_jobs() -> void
{
    spdlog::info("Killing all active sync jobs");

    const std::lock_guard<std::mutex> jobLock(m_JobMutex);
    for ([[maybe_unused]] const auto& [processID, jobDetails] : m_ActiveJobs)
    {
        kill_job(processID);
    }

    spdlog::info("All active syncs have been killed!");
}

auto JobManager::register_job(
    const std::string& jobName,
    const ::pid_t      processID,
    const int          stdoutPipe,
    const int          stderrPipe
) -> void
{
    spdlog::debug(
        "Registering job {} with the Job Manager. (pid: {})",
        jobName,
        processID
    );

    const std::lock_guard<std::mutex> jobLock(m_JobMutex);
    m_ActiveJobs.emplace(
        processID,
        SyncJob { .jobName    = jobName,
                  .stdoutPipe = stdoutPipe,
                  .stderrPipe = stderrPipe,
                  .startTime  = std::chrono::system_clock::now() }
    );
}

auto JobManager::deregister_jobs(const std::vector<::pid_t>& completedJobs)
    -> void
{
    if (completedJobs.empty())
    {
        spdlog::trace("No jobs to deregister");
        return;
    }

    spdlog::trace("Deregistering {} jobs", completedJobs.size());

    const std::lock_guard<std::mutex> jobLock(m_JobMutex);
    for (const auto& job : completedJobs)
    {
        m_ActiveJobs.erase(job);
    }
}

// NOLINTBEGIN(*-easily-swappable-parameters)
auto JobManager::start_job(
    const std::string&           jobName,
    std::string                  command,
    const std::filesystem::path& passwordFile = {}
) -> bool
// NOLINTEND(*-easily-swappable-parameters)
{
    if (job_is_running(jobName))
    {
        spdlog::warn(
            "A job with the name \"{}\" already exists! Not starting "
            "a duplicate job",
            jobName
        );
        return false;
    }

    std::array<int, 2> stdoutPipes = { -1, -1 };
    std::array<int, 2> stderrPipes = { -1, -1 };

    int status = ::pipe(stdoutPipes.data());

    if (status != 0)
    {
        static std::string errorMessage(BUFSIZ, '\0');
        errorMessage.clear();

        spdlog::warn(
            "Failed to create pipe for child stdout while syncing project "
            "{}! Error message: {}",
            jobName,
            ::strerror_r(errno, errorMessage.data(), errorMessage.size())
        );
    }

    status = ::pipe(stderrPipes.data());

    if (status != 0)
    {
        static std::string errorMessage(BUFSIZ, '\0');
        errorMessage.clear();

        spdlog::warn(
            "Failed to create pipe for child stderr while syncing project "
            "{}! Error message: {}",
            jobName,
            ::strerror_r(errno, errorMessage.data(), errorMessage.size())
        );
    }

    const ::pid_t pid = ::fork();

    if (pid == 0) // Child Process
    {
        // Close read end of the stdout pipe in the child process
        ::close(stdoutPipes.at(0));
        ::dup2(stdoutPipes.at(1), STDOUT_FILENO);

        // Close read end of the stderr pipe in the child process
        ::close(stderrPipes.at(0));
        ::dup2(stderrPipes.at(1), STDERR_FILENO);

        if (std::filesystem::exists(passwordFile)
            && std::filesystem::is_regular_file(passwordFile))
        {
            std::string   syncPassword;
            std::ifstream passwordFileStream(passwordFile);

            if (!passwordFileStream.good())
            {
                spdlog::error("Failed to read password file for {}!", jobName);
                return false;
            }

            passwordFileStream >> syncPassword;

            // Put rsync password into the child process' environment
            // NOLINTNEXTLINE(concurrency-mt-unsafe, misc-include-cleaner)
            ::putenv(std::format("RSYNC_PASSWORD={}", syncPassword).data());
        }

        auto* argv0 = ::strdup("/bin/sh");
        auto* argv1 = ::strdup("-c");

        const std::array<char*, 3> argv = { argv0, argv1, command.data() };

        ::execv(argv.at(0), argv.data());

        // If we get here `::execv()` failed
        static std::string errorMessage(BUFSIZ, '\0');
        errorMessage.clear();

        spdlog::error(
            "Call to execv() failed while trying to sync {}! Error message: {}",
            jobName,
            ::strerror_r(errno, errorMessage.data(), errorMessage.size())
        );

        // Keep memory from leaking in the event that exec fails
        // NOLINTBEGIN(*-no-malloc, *-owning-memory)
        ::free(argv0);
        ::free(argv1);
        // NOLINTEND(*-no-malloc, *-owning-memory)

        return false;
    }
    else if (pid == -1)
    {
        static std::string errorMessage(BUFSIZ, '\0');
        errorMessage.clear();

        spdlog::error(
            "Failed to fork process for project `{}`! Error message: "
            "{}",
            jobName,
            ::strerror_r(errno, errorMessage.data(), errorMessage.size())
        );
        return false;
    }

    // Close write end of the stdout pipe in the parent process
    ::close(stdoutPipes.at(1));

    // Close write end of the stderr pipe in the parent process
    ::close(stderrPipes.at(1));

    register_job(jobName, pid, stdoutPipes.at(1), stderrPipes.at(1));
    return true;
}

} // namespace mirror::sync_scheduler
