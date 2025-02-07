#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <signal.h>

#include <nlohmann/json.hpp>

#include <mirror/logger.hpp>
#include <mirror/sync_scheduler/queue.hpp>
#include <mirror/sync_scheduler/schedule.hpp>
#include <mirror/sync_scheduler/utils.hpp>

// used to stop the program cleanly when process is given a `SIGINT` signal
static bool keepRunning = true;

// used by the SIGINT signal handler to end the program
auto sigint_handler(int signal) -> void
{
    keepRunning = false;
}

auto main() -> int
{
    // ctrl c signal handler
    ::signal(SIGINT, sigint_handler);

    // read env data in from env.json
    auto env = mirror::sync_scheduler::read_JSON_from_file(
        "configs/sync-scheduler-env.json"
    );

    // read mirror data in from mirrors.json
    // ?: Does this need to be read in main?
    auto config
        = mirror::sync_scheduler::read_JSON_from_file("configs/mirrors.json");

    // initialize and configure connection to log server
    auto* logger = mirror::Logger::getInstance();
    logger->configure(
        env.at("logServerPort"),
        "Sync Scheduler",
        env.at("logServerHost")
    );

    // create and build new schedule
    auto* schedule = mirror::sync_scheduler::Schedule::getInstance();
    // build the schedule based on the mirrors.json config
    schedule->build(config.at("mirrors"));

    // create a pointer to the job queue class
    auto* queue = mirror::sync_scheduler::Queue::getInstance();
    // set queue dryrun
    queue->setDryrun(env.at("dryrun"));
    // generate the sync command maps
    queue->createSyncCommandMap(config.at("mirrors"));
    // start the queue (parameter is number of queue threads)
    queue->startQueue(env.at("queueThreads"));

    // keep alive thread
    // !: Not needed if the log server is being removed
    std::thread keepAliveThread(mirror::sync_scheduler::keep_alive_thread);

    // cin thread for manual sync
    // !: Not needed if manual syncs can be requested over a socket from the
    // website
    std::thread stdinThread(mirror::sync_scheduler::cin_thread);

    // update schedule thread
    // !: Should be made into a call back function that is called when a common
    // source of truth for mirrors.json detects a change to the file
    std::thread updateScheduleThread(
        mirror::sync_scheduler::update_schedule_thread
    );

    std::vector<std::string>* name; // ?: name of what?
    std::chrono::seconds      secondsToSleep;
    while (true)
    {
        // get the name of the next job and how long we have to sleep till the
        // next job from the schedule
        name = schedule->nextJob(secondsToSleep);

        // reset reloaded flag
        schedule->reloaded = false;

        // sleep for "secondsToSleep" seconds checking periodically for
        // mirrors.json reloads or exit code
        std::chrono::seconds secondsPassed(0);

        // interval between checks for reload and exit code
        std::chrono::seconds interval(1);
        while (secondsPassed <= secondsToSleep)
        {
            std::this_thread::sleep_for(std::chrono::seconds(interval));
            secondsPassed += interval;
            if (schedule->reloaded || !keepRunning)
            {
                break;
            }
        }
        if (!keepRunning)
        {
            break;
        }
        if (schedule->reloaded)
        {
            continue;
        }

        // add job names to job queue
        queue->push_back_list(name);
    }

    // program cleanup
    logger->info("Shutting down gracefully...");
    // make sure there is enough time for the logger to send a message before
    // freeing.
    std::this_thread::sleep_for(std::chrono::seconds(1));
    logger->close();

    // * Technically not required since the program is going to be ending in the
    // following line and the OS will automatically reclaim the memory
    delete schedule;
    delete queue;

    return 0;
}
