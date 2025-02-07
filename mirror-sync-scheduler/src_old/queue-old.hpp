#pragma once

#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include <mirror/logger.hpp>

namespace mirror::sync_scheduler
{
class Queue
{
  public: // Constructors
    Queue(Queue&)                   = delete;
    Queue(Queue&&)                  = delete;
    Queue& operator=(const Queue&)  = delete;
    Queue& operator=(const Queue&&) = delete;

  public: // Static Methods
    /**
     * @brief create the queue singleton object the first execution and return
     * the same object with every sequential execution
     *
     * @return pointer to queue singleton object
     */
    static auto getInstance() -> Queue*;

  public: // Methods
    /**
     * @brief add a vector of jobs to the back of the queue
     *
     * @param name pointer to a vector of job names
     */
    auto push_back_list(const std::vector<std::string>& name) -> void;

    /**
     * @brief sync a project in a new separate thread from the queue (similar to
     * jobQueueThread)
     *
     * @param name Name of the project to be synced
     */
    auto manual_sync(const std::string& name) -> void;

    /**
     * @brief start syncing projects from the queue
     *
     * @param maxThreads number of jobQueueThreads to create to sync jobs in
     * parallel
     */
    auto startQueue(const std::size_t maxThreads) -> void;

    /**
     * @brief Generate sync commands for every mirror entry in mirrors.json
     * using generateSyncCommands and store the commands in the syncCommands map
     *
     * @param config reference to mirrors.json object
     */
    auto createSyncCommandMap(const nlohmann::json& config) -> void;

    /**
     * @brief set the dryrun flag which controls if syncs are echoed or not.
     *
     * @param dryrun true causes syncs to be "echoed"
     */
    auto setDryrun(const bool dryrun) -> void;

  private: // Constructors
    /**
     * @brief Default constructor
     */
    Queue();

  private: // Methods
    /**
     * @brief syncs jobs from the queue in parallel with other jobQueueThreads
     */
    auto jobQueueThread() -> void;

    /**
     * @brief used to sync a given project with the commands from the
     * syncCommandMap
     *
     * @param name project to sync
     */
    auto syncProject(const std::string& name) -> void;

    /**
     * @brief generate rsync or script commands to sync a specific project
     *
     * @param config reference to mirrors.json object
     * @param name project to generate commands for
     * @return vector of commands needed to sync a project
     */
    auto
    generateSyncCommands(const nlohmann::json& config, const std::string& name)
        -> std::vector<std::string>;

    /**
     * @brief create a rsync command inside generateSyncCommands
     *
     * @param config reference to a rsync json object
     * @param options reference to the options string for this command
     * @return rsync command string
     */
    auto rsync(const nlohmann::json& config, const std::string& options) const
        -> std::string;

  private: // Members
    /**
     * @brief thread lock to prevent modifying data from multiple sync threads
     * at the same time
     */
    std::mutex m_ThreadLock;

    /**
     * @brief deque of queued jobes to be synced
     */
    std::deque<std::string> m_Queue;

    /**
     * @brief keep track of what jobs we are currently syncing so that we never
     * sync the same task in a different thread at the same time.
     */
    std::vector<std::string> m_CurrentJobs;

    /**
     * @brief map of projects to the commands needed to sync the project
     */
    std::unordered_map<std::string, std::vector<std::string>> m_SyncCommands;

    /**
     * @brief map of password files
     */
    std::unordered_map<std::string, std::string> m_PasswordFiles;

    /**
     * @brief used to prevent the queue from being started more than once
     */
    bool m_QueueRunning;

    /**
     * @brief used to run program as a "dry run"
     */
    bool m_DoingDryrun;

    /**
     * @brief connection to log server
     */
    mirror::Logger* m_Logger = mirror::Logger::getInstance();
};
} // namespace mirror::sync_scheduler
