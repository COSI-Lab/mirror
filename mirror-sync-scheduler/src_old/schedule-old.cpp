#include <mirror/sync_scheduler/schedule.hpp>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <mirror/logger.hpp>

#include <nlohmann/json.hpp>

namespace mirror::sync_scheduler
{ // private constructor for schedule class

Schedule::Schedule()
    : iterator(0)
{
}

// create an instance of Schedule the first time its ran on the heap
// every other time its ran it returns that same instance
auto Schedule::getInstance() -> Schedule*
{
    // a static variable is not updated when getInstance is called a second time
    static auto* schedule = new Schedule;
    return schedule;
}

auto Schedule::build(nlohmann::json& config) -> void
{
    // clear the jobs vector of any old jobs
    jobs.clear();
    iterator = 0;

    // create Task vector from mirrors.json
    std::vector<Task> tasks = parseTasks(config);

    // compute the least common multiple of all sync frequencies
    std::uint32_t leastCommonMultiple = 1;
    for (std::size_t idx = 0; idx < tasks.size(); idx++)
    {
        int syncs = tasks.at(idx).syncs;
        int a;
        int b;
        int remainder;

        if (leastCommonMultiple > syncs)
        {
            a = leastCommonMultiple;
            b = syncs;
        }
        else
        {
            a = syncs;
            b = leastCommonMultiple;
        }
        while (b != 0)
        {
            remainder = a % b;
            a         = b;
            b         = remainder;
        }
        // a is now the greatest common denominator;
        leastCommonMultiple = leastCommonMultiple * syncs / a;
    }

    double interval = 1.0 / leastCommonMultiple;
    for (std::uint32_t idx = 0; idx < leastCommonMultiple; idx++)
    {
        // std::cout << "---------" << i << "-------------" << std::endl;
        // create a job with all tasks who's syncs per day lines up with the
        // currient fraction of the lcm
        Job job;
        for (std::size_t jdx = 0; jdx < tasks.size(); jdx++)
        {
            Task task = tasks.at(jdx);
            if (idx % (leastCommonMultiple / task.syncs) == 0)
            {
                job.name.emplace_back(task.name);
            }
        }
        // set job time based on interval and i
        job.target_time = interval * idx;
        jobs.emplace_back(job);
    }

    // verify that the schedule works
    if (verifySchedule(tasks))
    {
        logger->info("Created and verified sync schedule.");
    }
    else
    {
        logger->error("Failed to create or verify sync schedule.");
    }
}

// verify that the job schedule schedules each job the correct number of times.
// verify that the job.start_time increases for each job and 0 <= start_time <=
// 1
auto Schedule::verifySchedule(const std::vector<Task>& tasks) -> bool
{
    // create Task vector from mirrors.json
    // create a map of tasks
    std::map<std::string, int> taskMap;
    for (std::size_t idx = 0; idx < tasks.size(); idx++)
    {
        taskMap.at(tasks.at(idx).name) = 0;
    }

    double previousStartTime = 0.0;
    for (std::size_t idx = 0; idx < jobs.size(); idx++)
    {
        // check that start_time increases and that 0.0 <= start_time <= 1.0
        if (!(previousStartTime <= jobs.at(idx).target_time <= 1.0))
        {
            return false;
        }
        previousStartTime = jobs.at(idx).target_time;

        // increment the appropriate task in our map for each name in our job
        // name vector
        for (std::size_t jdx = 0; jdx < jobs.at(idx).name.size(); jdx++)
        {
            taskMap.find(jobs.at(idx).name.at(jdx))->second++;
        }
    }

    // check that each job is scheduled the correct number of times
    for (std::size_t idx = 0; idx < tasks.size(); idx++)
    {
        if (tasks.at(idx).syncs != taskMap.find(tasks.at(idx).name)->second)
        {
            return false;
        }
    }

    return true;
}

// TODO: See if there is a better return value for this function
auto Schedule::nextJob(int& seconds_to_sleep) -> std::vector<std::string>*
{
    constexpr auto total_seconds_day
        = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::days(1)
        );

    const auto now = std::chrono::utc_clock::now();
    const auto time_since_midnight
        = std::chrono::duration_cast<std::chrono::seconds>(
            now - std::chrono::floor<std::chrono::days>(now)
        );

    // convert seconds_since_midnight to position in the schedule (0.0 <=
    // scheduleTime <= 1.0)
    const auto scheduleTime = time_since_midnight / total_seconds_day;

    /*
     * Find the first job that is greater than the current time.
     *
     * We start at `iterator` so that we dont have to search the entire job
     * array each time. We do it in this way so that if we start the program in
     * the middle of the day it will still select the correct job. In normal
     * operation, it will just go through the loop once increasing the iterator
     * to the next job.
     */
    while (iterator < jobs.size()
           && jobs.at(iterator).target_time <= scheduleTime)
    {
        iterator++;
    }

    // if we are at the end of the schedule
    if (iterator == jobs.size())
    {
        // reset the iterator
        iterator = 0;

        // sleep till midnight
        // total seconds in a day - current time in seconds cast to an integer
        // and added 1

        // ** SIDE EFFECT **
        // TODO: Replace with something different
        seconds_to_sleep = static_cast<int>(
            (total_seconds_day - (scheduleTime * total_seconds_day)) + 1
        );
        return &(jobs.at(jobs.size() - 1).name);
    }

    // else sleep till next job
    // target time in seconds - currient time in seconds cast to an integer and
    // added 1

    // ** SIDE EFFECT **
    // TODO: Replace with something different
    seconds_to_sleep = static_cast<int>(
        ((jobs.at(iterator).target_time * total_seconds_day)
         - (scheduleTime * total_seconds_day))
        + 1
    );
    return &(jobs.at(iterator - 1).name);
}

// read mirrors.json into a list of tasks
auto Schedule::parseTasks(nlohmann::json& config) -> std::vector<Task>
{
    std::vector<Task> tasks;
    // create a vector of task structs from mirrors.json
    for (const auto& x : config.items())
    {
        nlohmann::json Xvalue = x.value();
        nlohmann::json rsync  = Xvalue.at("rsync");
        nlohmann::json script = Xvalue.at("script");

        if (!rsync.is_null())
        {
            Task task;
            task.name  = x.key();
            task.syncs = rsync.value("syncs_per_day", 0);
            tasks.emplace_back(task);
        }
        else if (!script.is_null())
        {
            Task task;
            task.name  = x.key();
            task.syncs = script.value("syncs_per_day", 0);
            tasks.emplace_back(task);
        }
    }

    return tasks;
}
} // namespace mirror::sync_scheduler
