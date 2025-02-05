#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <signal.h>
#include <sys/stat.h>

#include <mirror/logger.hpp>
#include <nlohmann/json.hpp>

#include "queue.hpp"
#include "schedule.hpp"
#include "utils.hpp"

// used to stop the program cleanly with ctrl c
static bool keepRunning = true;

// used by the SIGINT signal handler to end the program
auto intHandler(int i) -> void
{
    keepRunning = false;
}

auto main() -> int
{
    // ctrl c signal handler
    signal(SIGINT, intHandler);

    // read env data in from env.json
    nlohmann::json env = readJSONFromFile("configs/sync-scheduler-env.json");

    // read mirror data in from mirrors.json
    nlohmann::json config = readJSONFromFile("configs/mirrors.json");

    // initialize and configure connection to log server
    mirror::Logger* logger = mirror::Logger::getInstance();
    logger->configure(
        env.at("logServerPort"),
        "Sync Scheduler",
        env.at("logServerHost")
    );

    // create and build new schedule
    Schedule* schedule = Schedule::getInstance();
    // build the schedule based on the mirrors.json config
    schedule->build(config.at("mirrors"));

    // create a pointer to the job queue class
    Queue* queue = Queue::getInstance();
    // set queue dryrun
    queue->setDryrun(env.at("dryrun"));
    // generate the sync command maps
    queue->createSyncCommandMap(config.at("mirrors"));
    // start the queue (parameter is number of queue threads)
    queue->startQueue(env.at("queueThreads"));

    // keep alive thread
    std::thread kt(keep_alive_thread);

    // cin thread for manual sync
    std::thread ct(cin_thread);

    // update schedule thread
    std::thread ust(update_schedule_thread);

    std::vector<std::string>* name;
    int                       seconds_to_sleep;
    while (true)
    {
        // get the name of the next job and how long we have to sleep till the
        // next job from the schedule
        name = schedule->nextJob(seconds_to_sleep);

        /*
            // print the next jobs and the time to sleep
            for (int idx = 0; idx < name->size(); idx++)
            {
                std::cout << name->at(idx) << " " << std::endl;
            }
            std::cout << seconds_to_sleep << std::endl;
         */

        // reset reloaded flag
        schedule->reloaded = false;

        // sleep for "seconds_to_sleep" seconds checking periodically for
        // mirrors.json reloads or exit code
        std::chrono::seconds secondsPassed(0);

        // interval between checks for reload and exit code
        std::chrono::seconds interval(1);
        while (secondsPassed <= seconds_to_sleep)
        {
            std::this_thread::sleep_for(std::chrono::seconds(interval));
            secondsPassed += interval;
            if (schedule->reloaded)
            {
                break;
            }
            if (!keepRunning)
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
    delete schedule;
    delete queue;

    return 0;
}
