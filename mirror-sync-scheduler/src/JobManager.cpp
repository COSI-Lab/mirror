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
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <stop_token>
#include <string>
// NOLINTNEXTLINE(*-deprecated-headers, llvm-include-order)
#include <string.h> // Required for `::strerror_r`. Not available in `cstring`
#include <thread>
#include <vector>

// Third Party Includes
#include <spdlog/spdlog.h>

namespace mirror::sync_scheduler
{
JobManager::JobManager()
{
    m_ProcessReaper = std::jthread(
        [this](const std::stop_token& stopToken) -> void
        {
            while (true)
            {
                if (stopToken.stop_requested())
                {
                    spdlog::info("Process reaper thread stop requested");
                    kill_all_jobs();

                    return;
                }

                auto completedJobs = reap_processes();

                deregister_jobs(completedJobs);
                completedJobs.clear();

                spdlog::trace("Process reaper thread going to sleep");
                std::unique_lock<std::mutex> reaperLock(m_ReaperMutex);
                m_SleepVariable.wait_for(reaperLock, std::chrono::minutes(1));
                spdlog::trace("Process reaper thread woke up");
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

auto JobManager::reap_processes() -> std::vector<std::string>
{
    static std::string errorMessage(BUFSIZ, '\0');

    std::vector<std::string> completedJobs;
    completedJobs.reserve(m_ActiveJobs.size());

    const std::lock_guard<std::mutex> JobLock(m_JobMutex);
    for (const auto& [jobName, job] : m_ActiveJobs)
    {
        int exitCode = 0;

        // NOLINTBEGIN(misc-include-cleaner)
        const ::pid_t status = ::waitpid(job.processID, &exitCode, WNOHANG);
        // NOLINTEND(misc-include-cleaner)

        if (status == job.processID) // Process finished executing
        {
            if (exitCode == EXIT_SUCCESS)
            {
                spdlog::info("Project `{}` successfully synced!", jobName);
            }
            else
            {
                spdlog::warn(
                    "Project `{}` failed to sync! Exit code: {}",
                    jobName,
                    exitCode
                );
            }

            completedJobs.emplace_back(jobName);
        }
        else if (status == 0) // Process still running
        {
            const auto syncDuration
                = std::chrono::system_clock::now() - job.startTime;
            if (syncDuration < std::chrono::hours(1))
            {
                continue;
            }

            spdlog::warn(
                "Project `{}` has been syncying for at least 1 "
                "hour. Process may be hanging, killing "
                "process.",
                jobName
            );

            kill_job(jobName);

            completedJobs.emplace_back(jobName);
        }
        else if (status == -1) // waitpid() failed
        {
            spdlog::error(
                "waitpid() returned -1! Error message: {}",
                ::strerror_r(errno, errorMessage.data(), errorMessage.size())
            );
            completedJobs.emplace_back(jobName);
        }
    }

    return completedJobs;
}

auto JobManager::kill_job(const std::string& jobName) -> void
{
    const auto& job = m_ActiveJobs.at(jobName);

    // NOLINTNEXTLINE(misc-include-cleaner)
    int status = ::kill(job.processID, SIGKILL);

    if (status == 0)
    {
        spdlog::info(
            "Successfully sent process {} ({}) a SIGKILL",
            job.processID,
            jobName
        );
    }
    else
    {
        static std::string errorMessage(BUFSIZ, '\0');

        spdlog::error(
            "Failed to send process {} ({}) a SIGKILL! "
            "Error message: {}",
            job.processID,
            jobName,
            ::strerror_r(errno, errorMessage.data(), errorMessage.size())
        );
    }

    int exitCode = -1;
    status       = ::waitpid(job.processID, &exitCode, 0);

    if (status == job.processID)
    {
        spdlog::info("Process {} successfully reaped", job.processID);
    }
    else
    {
        spdlog::error("Failed to reap process {}!", job.processID);
    }
}

auto JobManager::kill_all_jobs() -> void
{
    spdlog::info("Killing all active sync jobs!");

    const std::lock_guard<std::mutex> jobLock(m_JobMutex);
    for ([[maybe_unused]] const auto& [jobName, jobDetails] : m_ActiveJobs)
    {
        kill_job(jobName);
    }
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
        jobName,
        SyncJob { .processID  = processID,
                  .stdoutPipe = stdoutPipe,
                  .stderrPipe = stderrPipe,
                  .startTime  = std::chrono::system_clock::now() }
    );
}

auto JobManager::deregister_jobs(const std::vector<std::string>& completedJobs)
    -> void
{
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
    if (m_ActiveJobs.contains(jobName))
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

            // NOLINTNEXTLINE(concurrency-mt-unsafe, misc-include-cleaner)
            ::putenv(std::format("RSYNC_PASSWORD={}", syncPassword).data());
        }

        auto* argv0 = ::strdup("/bin/sh");
        auto* argv1 = ::strdup("-c");

        const std::array<char*, 3> argv = { argv0, argv1, command.data() };

        ::execv(argv.at(0), argv.data());

        // If we get here `::execv()` failed
        static std::string errorMessage(BUFSIZ, '\0');

        spdlog::error(
            "Call to execv() failed while trying to sync {}! Error message: {}",
            jobName,
            ::strerror_r(errno, errorMessage.data(), errorMessage.size())
        );

        // Keep memory from leaking in the event that exec fails
        free(argv0);
        free(argv1);

        return false;
    }
    else if (pid == -1)
    {
        static std::string errorMessage(BUFSIZ, '\0');

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
