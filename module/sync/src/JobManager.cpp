/**
 * @file JobManager.cpp
 * @author Cary Keesler
 * @brief
 */

// Header Being Defined
#include <mirror/sync_scheduler/JobManager.hpp>

// System Includes
#include <sys/ioctl.h>
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
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <mutex>
#include <random>
#include <stop_token>
#include <string>
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
    m_ProcessReaper = std::jthread(&JobManager::process_reaper, this);
}

JobManager::~JobManager()
{
    spdlog::info("Joining process reaper thread");
    m_ProcessReaper.request_stop();
    m_SleepVariable.notify_all();
    m_ProcessReaper.join();
    spdlog::info("Process reaper thread joined!");
}

auto JobManager::process_reaper(const std::stop_token& stopToken) -> void
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
            this->kill_all_jobs();

            return;
        }

        auto completedJobs = reap_processes();

        this->deregister_jobs(completedJobs);
        completedJobs.clear();
    }
}

auto JobManager::get_child_process_ids(const ::pid_t processID = ::getpid())
    -> std::vector<::pid_t>
{
    static const std::filesystem::path taskDirectory
        = std::filesystem::absolute(std::format("/proc/{}/task/", processID));

    std::vector<::pid_t> childProcesses = {};
    for (const auto& taskEntry :
         std::filesystem::directory_iterator(taskDirectory))
    {
        if (!std::filesystem::is_directory(taskEntry))
        {
            continue;
        }

        std::ifstream childrenFile(taskEntry.path() / "children");

        if (!childrenFile.good())
        {
            spdlog::error(
                "Failed to open children file! Path: {}",
                (taskEntry.path() / "children").string()
            );

            continue;
        }

        childProcesses.insert(
            childProcesses.end(),
            std::istream_iterator<::pid_t>(childrenFile),
            {}
        );
    }

    return childProcesses;
}

auto JobManager::reap_processes() -> std::vector<::pid_t>
{
    std::string errorMessage(BUFSIZ, '\0');

    static const ::pid_t syncSchedulerProcessID = ::getpid();

    std::vector<::pid_t> completedJobs;
    completedJobs.reserve(m_ActiveJobs.size());

    const std::lock_guard<std::mutex> JobLock(m_JobMutex);
    for (const ::pid_t childProcessID : JobManager::get_child_process_ids())
    {
        spdlog::trace(
            "Attempting to wait on process with pid {}",
            childProcessID
        );

        int                status      = 0;
        const bool         isKnownJob  = m_ActiveJobs.contains(childProcessID);
        constexpr auto     JOB_TIMEOUT = std::chrono::hours(6);
        std::chrono::hours syncDuration;

        // NOLINTNEXTLINE(misc-include-cleaner)
        switch (::waitpid(childProcessID, &status, WNOHANG))
        {
        case -1: // waitpid() failed
            spdlog::error(
                "waitpid() returned -1 for process with pid: {}! Error "
                "message: {}",
                // NOLINTNEXTLINE(*-include-cleaner)
                ::strerror_r(errno, errorMessage.data(), errorMessage.size()),
                childProcessID
            );
            completedJobs.emplace_back(childProcessID);
            break;

        case 0: // Process still running
            if (!isKnownJob)
            {
                break;
            }

            syncDuration = std::chrono::duration_cast<std::chrono::hours>(
                std::chrono::system_clock::now()
                - m_ActiveJobs.at(childProcessID).startTime
            );
            if (syncDuration < JOB_TIMEOUT)
            {
                break;
            }

            spdlog::warn(
                "Project {} has been syncing for at least {} hour{}. "
                "Process "
                "may be hanging, attempting to send SIGTERM. (pid: {})",
                m_ActiveJobs.at(childProcessID).jobName,
                JOB_TIMEOUT.count(),
                // if not one hour, plural
                (JOB_TIMEOUT.count() == 1 ? "" : "s"),
                childProcessID
            );

            JobManager::interrupt_job(childProcessID);

            completedJobs.emplace_back(childProcessID);
            break;

        default:
            // NOLINTNEXTLINE(*-include-cleaner)
            const int exitStatus = WEXITSTATUS(status);

            if (isKnownJob && exitStatus == EXIT_SUCCESS)
            {
                spdlog::info(
                    "Project {} successfully synced! (pid: {})",
                    m_ActiveJobs.at(childProcessID).jobName,
                    childProcessID
                );
                completedJobs.emplace_back(childProcessID);
            }
            else if (isKnownJob && exitStatus != EXIT_SUCCESS)
            {
                const auto logFileName
                    = this->write_fail_to_file(childProcessID);

                spdlog::warn(
                    "Project {} failed to sync! See {}. Exit code: {} (pid: {})",
                    m_ActiveJobs.at(childProcessID).jobName,
                    logFileName,
                    exitStatus,
                    childProcessID
                );
                completedJobs.emplace_back(childProcessID);
            }
            else if (!isKnownJob && exitStatus == EXIT_SUCCESS)
            {
                spdlog::info(
                    "Reaped successful unregistered child process with pid {}",
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
    }

    return completedJobs;
}

auto JobManager::write_fail_to_file(const ::pid_t processID) -> std::string
{
    const auto startTime            = m_ActiveJobs.at(processID).startTime;
    const std::time_t tStartTime    = std::chrono::system_clock::to_time_t(startTime);
    const auto tmStartTime          = std::localtime(&tStartTime);

    char timestamp[14];
    std::strftime(timestamp,sizeof(timestamp),"%Y%j%H%M%S",tmStartTime);

    const auto errorLogPath = std::filesystem::relative("error-logs");

    // only one process can run for a given jobName at a time so we know
    // logFileName will be unique
    auto logFileName = std::format(
        "{}-{}.log",
        m_ActiveJobs.at(processID).jobName,
        timestamp
    );

    std::ofstream logFile(errorLogPath / logFileName);

    int bytesAvailable = 0;
    ::ioctl(m_ActiveJobs.at(processID).stdPipe, FIONREAD, &bytesAvailable);

    std::string pipeContents;
    pipeContents.resize(bytesAvailable + 1);

    ::read(
        m_ActiveJobs.at(processID).stdPipe,
        pipeContents.data(),
        bytesAvailable
    );

    logFile << pipeContents;

    return logFileName;
}

// NOLINTNEXTLINE(*-no-recursion)
auto JobManager::interrupt_job(const ::pid_t processID) -> void
{
    // Interrupt child processes recursively. Starts with the grandest child and
    // works its way back up to the direct decendant of the sync scheduler
    //
    // Base case: process with no children. `get_child_process_ids` will be an
    // empty collection meaning nothing to iterate over
    for (const ::pid_t childProcessID : JobManager::get_child_process_ids())
    {
        JobManager::interrupt_job(childProcessID);
    }

    // NOLINTNEXTLINE(misc-include-cleaner)
    const int killReturn = ::kill(processID, SIGTERM);

    if (killReturn != 0)
    {
        std::string errorMessage(BUFSIZ, '\0');

        spdlog::error(
            "Failed to send process {} a SIGTERM! Error message: {}",
            processID,
            // NOLINTNEXTLINE(*-include-cleaner)
            ::strerror_r(errno, errorMessage.data(), errorMessage.size())
        );

        return;
    }

    spdlog::debug("Successfully sent process {} a SIGTERM", processID);

    constexpr auto SIGTERM_TIMEOUT = std::chrono::seconds(30);
    const auto     start           = std::chrono::system_clock::now();
    auto           now             = start;

    while ((now - start) < SIGTERM_TIMEOUT)
    {
        const int waitReturn = ::waitpid(processID, nullptr, WNOHANG);

        if (waitReturn == processID)
        {
            spdlog::trace("Process {} successfully reaped", processID);
            return;
        }

        constexpr auto CHECK_INTERVAL = std::chrono::milliseconds(100);
        std::this_thread::sleep_for(CHECK_INTERVAL);

        now = std::chrono::system_clock::now();
    }

    spdlog::error("Failed to terminate process {} with SIGTERM", processID);

    JobManager::kill_job(processID);
}

auto JobManager::job_is_running(const std::string& jobName) -> bool
{
    return std::ranges::any_of(
        m_ActiveJobs,
        [&jobName](const auto& job) -> bool
        {
            const auto& [processID, jobDetails] = job;
            return jobDetails.jobName == jobName;
        }
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
        std::string errorMessage(BUFSIZ, '\0');

        spdlog::error(
            "Failed to send process {} a SIGKILL! Error message: {}",
            processID,
            // NOLINTNEXTLINE(*-include-cleaner)
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
        JobManager::kill_job(processID);
    }

    spdlog::info("All active syncs have been killed!");
}

auto JobManager::register_job(
    const std::string& jobName,
    const ::pid_t      processID,
    const int          stdPipe
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
                  .stdPipe = stdPipe,
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

    spdlog::debug("Deregistering {} jobs", completedJobs.size());

    const std::lock_guard<std::mutex> jobLock(m_JobMutex);
    for (const auto& job : completedJobs)
    {
        ::close(m_ActiveJobs.at(job).stdPipe);
        m_ActiveJobs.erase(job);
    }
}

// NOLINTBEGIN(*-easily-swappable-parameters)
auto JobManager::start_job(
    const std::string&           jobName,
    std::string                  command,
    const std::filesystem::path& passwordFile
) -> bool
// NOLINTEND(*-easily-swappable-parameters)
{
    if (this->job_is_running(jobName))
    {
        spdlog::warn(
            "A job with the name \"{}\" already exists! Not starting a "
            "duplicate job",
            jobName
        );
        return false;
    }

    std::array<int, 2> stdPipes = { -1, -1 };

    int status = ::pipe(stdPipes.data());

    if (status != 0)
    {
        std::string errorMessage(BUFSIZ, '\0');

        spdlog::warn(
            "Failed to create pipe for child stdout/stderr while syncing "
            "project {}! Error message: {}",
            jobName,
            // NOLINTNEXTLINE(*-include-cleaner)
            ::strerror_r(errno, errorMessage.data(), errorMessage.size())
        );
    }

    const ::pid_t pid = ::fork();

    if (pid == 0) // Child Process
    {
        // Close read end of the stdout/stderr pipe in the child process
        ::close(stdPipes.at(0));

        if (std::filesystem::exists(passwordFile)
            && std::filesystem::is_regular_file(passwordFile))
        {
            std::string   syncPassword;
            std::ifstream passwordFileStream(passwordFile);

            if (!passwordFileStream.good())
            {
                spdlog::error("Failed to read password file for {}!", jobName);
                // NOLINTNEXTLINE(*-mt-unsafe)
                std::exit(EXIT_FAILURE);
            }

            passwordFileStream >> syncPassword;

            // Put rsync password into the child process' environment
            spdlog::trace("Putting rsync password into child environment");
            // NOLINTNEXTLINE(*-mt-unsafe, *-include-cleaner)
            ::setenv(
                "RSYNC_PASSWORD",
                syncPassword.c_str(),
                static_cast<int>(false)
            );
        }

        //! ::strdup allocates memory with ::malloc. Typically this memory
        //! should be ::free'd. However, argv is supposed to have the same
        //! lifetime as the process it belongs to, therefore the memory should
        //! never be freed and we do not need to maintain a copy of the pointer
        //! to free it at a later time
        // NOLINTBEGIN(*-include-cleaner)
        const std::array<char*, 4> argv { ::strdup("/bin/sh"),
                                          ::strdup("-c"),
                                          ::strdup(command.data()),
                                          nullptr };
        // NOLINTEND(*-include-cleaner)

        spdlog::debug("Setting process group ID");
        ::setpgid(0, 0);

        // not quite yet :)
        spdlog::trace("Calling exec");

        // store the current stdout and stderr in case we need to restore them
        const int oldStdout = ::dup(STDOUT_FILENO);
        const int oldStderr = ::dup(STDERR_FILENO);

        // from now on send stdout and stderr to the pipe
        ::dup2(stdPipes.at(1), STDOUT_FILENO);
        ::dup2(stdPipes.at(1), STDERR_FILENO);

        // and call exec
        ::execv(argv.at(0), argv.data());

        // If we get here `::execv()` failed
        std::string errorMessage(BUFSIZ, '\0');

        // restore stdout and stderr so we can log the message
        ::dup2(oldStdout,STDOUT_FILENO);
        ::dup2(oldStderr,STDERR_FILENO);

        // close unused fds
        ::close(oldStdout);
        ::close(oldStderr);
        ::close(stdPipes.at(1));

        spdlog::error(
            "Call to execv() failed while trying to sync {}! Error message: {}",
            jobName,
            // NOLINTNEXTLINE(*-include-cleaner)
            ::strerror_r(errno, errorMessage.data(), errorMessage.size())
        );

        // NOLINTNEXTLINE(*-mt-unsafe)
        std::exit(EXIT_FAILURE);
    }
    // NOLINTNEXTLINE(*-else-after-return)
    else if (pid == -1)
    {
        std::string errorMessage(BUFSIZ, '\0');

        spdlog::error(
            "Failed to fork process for project `{}`! Error message: {}",
            jobName,
            // NOLINTNEXTLINE(*-include-cleaner)
            ::strerror_r(errno, errorMessage.data(), errorMessage.size())
        );
        return false;
    }

    // Close write end of the stdout/stderr pipe in the parent process
    ::close(stdPipes.at(1));

    this->register_job(jobName, pid, stdPipes.at(0));
    return true;
}
} // namespace mirror::sync_scheduler
