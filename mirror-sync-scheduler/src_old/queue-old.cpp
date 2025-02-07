#include <mirror/sync_scheduler/queue.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <mirror/logger.hpp>

#include <nlohmann/json.hpp>

namespace mirror::sync_scheduler
{
// private constructor for Queue class
Queue::Queue()
    : m_QueueRunning(false)
{
}

// create an instance of Queue the first time its ran on the heap
// every other time its ran it returns that same instance
auto Queue::getInstance() -> Queue*
{
    // a static variable is not updated when getInstance is called a second time
    static Queue* queue = new Queue;
    return queue;
}

// set the dryrun variable
auto Queue::setDryrun(const bool dryrun) -> void
{
    m_DoingDryrun = dryrun;
}

// used to add a list of jobs to the queue
auto Queue::push_back_list(const std::vector<std::string>& name) -> void
{
    m_ThreadLock.lock();
    // store old queue size for duplicate check later.
    auto oldQueueSize = m_Queue.size();

    // add name list to the queue
    for (std::size_t idx = 0; idx < name.size(); idx++)
    {
        auto search = m_SyncCommands.find(name.at(idx));
        if (search != m_SyncCommands.end())
        {
            // check to make sure that given string is in syncCommands
            m_Queue.emplace_back(name.at(idx));
        }
        else
        {
            m_Logger->warn(name.at(idx) + " is not valid");
        }
    }

    // duplicate check (only runs when queue wasnt empty to start with, so
    // should run rarely)
    for (std::size_t idx = 0; idx < oldQueueSize; idx++)
    {
        for (auto iter = std::begin(m_Queue) + oldQueueSize;
             iter != std::end(m_Queue);
             iter++)
        {
            if (m_Queue.at(idx) == *iter)
            {
                m_Queue.erase(iter);
                m_Logger->warn(
                    "erasing duplicate from queue: " + m_Queue.at(idx)
                );

                break;
            }
        }
    }

    m_ThreadLock.unlock();
}

// manually sync a project in a detached thread
auto Queue::manual_sync(const std::string& name) -> void
{
    // create a thread using a lambda function
    std::thread t(
        [=]
        {
            // lock thread
            m_ThreadLock.lock();

            // check to make sure that given string is in syncCommands
            if (m_SyncCommands.find(name) == m_SyncCommands.end())
            {
                m_Logger->warn(std::format(
                    "Sync requested for non-existent project `%s`",
                    name
                ));
                m_ThreadLock.unlock();
                return;
            }

            // make sure that the job is a not already syncing
            if (std::find(
                    std::begin(m_CurrentJobs),
                    std::end(m_CurrentJobs),
                    name
                )
                == m_CurrentJobs.end())
            {
                // add job to current jobs
                m_CurrentJobs.push_back(name);

                // unlock before running the job
                m_ThreadLock.unlock();
                // run the job
                syncProject(name);
                // lock after running the job
                m_ThreadLock.lock();

                // erase the job from the currientJobs vector
                m_CurrentJobs.erase(
                    std::find(m_CurrentJobs.begin(), m_CurrentJobs.end(), name)
                );
            }
            else
            {
                m_Logger->warn(
                    name + " is already syncing (manual sync failed)"
                );
            }

            // unlock thread
            m_ThreadLock.unlock();
        }
    );

    // detach the thread
    // TODO: Replace with thread pool
    // * We should stop detaching threads, I think it is better to hold onto
    // the thread handle
    t.detach();
}

// TODO: Make better thread pool
// creates maxThreads jobQueueThreads that sync projects from the queue
auto Queue::startQueue(const std::size_t maxThreads) -> void
{
    if (m_QueueRunning)
    {
        m_Logger->warn("startQueue tried to start a second time");
        return;
    }
    // create a pool of threads to run syncs in
    for (std::size_t idx = 0; idx < maxThreads; idx++)
    {
        std::thread t(&Queue::jobQueueThread, this);
        // * We should stop detaching threads, I think it is better to hold onto
        // the thread handle
        t.detach();
    }

    m_QueueRunning = true;
}

// syncs jobs from the queue in parallel with other jobQueueThreads
// ! Doesn't handle dispatching to a thread, this function is already running
// multiple times in parallel.
auto Queue::jobQueueThread() -> void
{
    while (true)
    {
        m_ThreadLock.lock();

        // If queue is empty, continue to next iteration of loop
        if (m_Queue.empty())
        {
            m_ThreadLock.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        // select and remove the first element from queue
        std::string jobName = m_Queue.front();
        m_Queue.pop_front();

        // check to make sure that jobName is not in currentJobs already
        if (std::find(m_CurrentJobs.begin(), m_CurrentJobs.end(), jobName)
            == m_CurrentJobs.end())
        {
            // add job to current jobs
            m_CurrentJobs.emplace_back(jobName);

            // unlock before running the job
            m_ThreadLock.unlock();

            // run the job within our threadpool
            // ? Does this actually run in a separate thread?
            syncProject(jobName);

            // lock after running the job
            m_ThreadLock.lock();

            // erase the job from the currientJobs vector
            m_CurrentJobs.erase(
                std::find(m_CurrentJobs.begin(), m_CurrentJobs.end(), jobName)
            );
        }

        m_ThreadLock.unlock();
        // sleep for 5 seconds so that we arent running constantly and to
        // prevent constant locking
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

// sync a project using the commands from the the syncCommands map
auto Queue::syncProject(const std::string& name) -> void
{
    m_Logger->info(name + " started");
    // used to get the status of rsync when it runs
    int status = -1;

    // for each command in the vector of commands for the given project in the
    // syncCommands map most projects have only one command but some have 2 or
    // even 3
    for (std::string command : m_SyncCommands.at(name))
    {
        // have the commands output to /dev/null so that we dont fill log files
        command += " > /dev/null";
        if (m_DoingDryrun)
        {
            command = std::format("echo \"%s\"", command);
            // run command
            // TODO: Replace use of `system()` with something with less overhead
            status  = ::system(command.c_str());
        }
        else if (m_PasswordFiles.find(name) != m_PasswordFiles.end())
        {
            // read password in from passwordFile
            std::ifstream passwordFile("configs/" + m_PasswordFiles.at(name));

            if (!passwordFile.good())
            {
                m_Logger->error(std::format(
                    "Failed to read password file for `%s`. Continuing",
                    name
                ));
                continue;
            }

            std::string password;
            std::getline(passwordFile, password);

            // create string with environment variable
            std::string passwordStr
                = std::format("RSYNC_PASSWORD=%s", password);

            // put password into command environment
            // ! This loads the password into the current process' environment,
            // it should not do that. I think we use `exec()` for handling
            // syncs.
            ::putenv(passwordStr.data());

            // run command
            // TODO: Replace use of `system()` with something with less overhead
            status = ::system(command.c_str());
        }
        else
        {
            // run command
            // TODO: Replace use of `system()` with something with less overhead
            status = ::system(command.c_str());
        }
    }

    // temporary sleep for testing when doing a dry run
    if (m_DoingDryrun)
    {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    if (status == EXIT_SUCCESS)
    {
        m_Logger->info(std::format("Successfully synced `%s` project", name));
    }
    else
    {
        m_Logger->error(std::format("Failed to sync `%s` project", name));
    }
}

// create a map that maps a task to the commands needed to sync it
auto Queue::createSyncCommandMap(const nlohmann::json& config) -> void
{
    // make sure the syncCommand and passwordFiles maps are empty
    m_SyncCommands.clear();
    m_PasswordFiles.clear();

    // loop through all mirror entries
    for (auto& item : config.items())
    {
        // generate sync commands for each entry and add it to our map
        // TODO: Something seems off here, figure out what
        m_SyncCommands.at(item.key())
            = generateSyncCommands(config.at(item.key()), item.key());
    }
}

// generate commands to sync a given project config
auto Queue::generateSyncCommands(
    const nlohmann::json& config,
    const std::string&    name
) -> std::vector<std::string>
{
    std::vector<std::string> output     = {};
    // check if project has an rsync json object
    auto                     rsyncData  = config.find("rsync");
    // check if project has a script json object
    auto                     scriptData = config.find("script");
    // (there is a chance that a project has neither rsync or script sync ie.
    // templeOS)

    if (rsyncData != config.end())
    {
        // run rsync and sync the project
        std::string options = config.at("rsync").value("options", "");
        output.emplace_back(rsync(config.at("rsync"), options));

        // 2 stage syncs happen sometimes
        options = config.at("rsync").value("second", "");
        if (!options.empty())
        {
            output.emplace_back(rsync(config.at("rsync"), options));
        }

        // a few mirrors are 3 stage syncs
        options = config.at("rsync").value("third", "");
        if (!options.empty())
        {
            output.emplace_back(rsync(config.at("rsync"), options));
        }

        // handle passwords
        options = config.at("rsync").value("password_file", "");
        if (!options.empty())
        {
            m_PasswordFiles.at(name) = options;
        }
    }
    // project uses script sync
    else if (scriptData != config.end())
    {
        std::string command = config.at("script").value("command", "");
        std::vector<std::string> arguments = config.at("script").value(
            "arguments",
            std::vector<std::string> {}
        );

        for (const std::string& arg : arguments)
        {
            command = std::format("%s %s", command, arg);
        }

        output.emplace_back(command);
    }

    return output;
}

// compose an rsync command
auto Queue::rsync(const nlohmann::json& config, const std::string& options)
    const -> std::string
{
    std::string command = std::format("rsync %s ", options);

    const std::string user = config.value("user", "");
    const std::string host = config.value("host", "");
    const std::string src  = config.value("src", "");
    const std::string dest = config.value("dest", "");

    if (!user.empty())
    {
        command = std::format("%s%s@%s::%s %s", command, user, host, src, dest);
        // command = command + user + "@" + host + "::" + src + " " + dest;
    }
    else
    {
        command = std::format("%s%s::%s %s", command, host, src, dest);
        // command = command + host + "::" + src + " " + dest;
    }

    return command;
}
} // namespace mirror::sync_scheduler
