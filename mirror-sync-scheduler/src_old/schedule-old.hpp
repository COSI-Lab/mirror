#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace mirror::sync_scheduler
{
/**
 * @brief Task structs are used in the build algorithm to represent a project
 */
struct Task
{
    /**
     * @brief name of project
     */
    std::string name;

    /**
     * @brief number of times the project wants to sync per day
     */
    std::int32_t syncs;
};

/**
 * @brief Job structs are created by the build algorithm to represent a job to
 * be run at a specific time
 */
struct Job
{
    /**
     * @brief vector of names of all the projects that will be queued
     */
    std::vector<std::string> name;

    /**
     * @brief double between 0 and 1 representing the fraction of a day till its
     * ran
     */
    double target_time;
};

/**
 * @brief
 *
 */
class Schedule
{
  private:
    Schedule();

  public:
    // delete copy and move constructors
    Schedule(Schedule&)                  = delete;
    Schedule(Schedule&&)                 = delete;
    Schedule& operator=(const Schedule&) = delete;
    Schedule& operator=(Schedule&&)      = delete;

    /**
     * @brief create the schedule singleton object the first execution and
     * return the same object with every sequential execution
     *
     * @return pointer to schedule singleton object
     */
    static auto getInstance() -> Schedule*;

    /**
     * @brief use the mirrors.json file to create the schedule by populating the
     * jobs vector
     *
     * @param config reference to mirrors.json object
     */
    auto build(nlohmann::json& config) -> void;

    /**
     * @brief calculate the next job to be updated using the schedule and the
     * time to till then
     *
     * @param seconds_to_sleep updated to the time to sleep till the next job
     * (pass by reference)
     *
     * @return pointer to a vector of strings with the names of the projects to
     * be updated next
     *
     * @note
     * ?: Why is this returning a pointer to a vector?
     *
     * TODO: Update the return type of this function so that the
     * `seconds_to_sleep` are not being passed out through a reference.
     * `std::pair` could be used but there is likely a deeper issue with the
     * structure of this function that should be addressed
     */
    auto nextJob(int& seconds_to_sleep) -> std::vector<std::string>*;

  private:
    /**
     * @brief used inside build to run several sanity checks for the schedule
     *
     * @param tasks vector of `Task` structs
     * @return whether the sanity checks passed or failed
     */
    auto verifySchedule(const std::vector<Task>& tasks) -> bool;

    /**
     * @brief used inside build to parse `mirrors.json` into a vector of Task
     * structs
     *
     * @param config reference to mirrors.json object
     * @return vector of Task structs for each project
     */
    auto parseTasks(nlohmann::json& config) -> std::vector<Task>;

  public:
    /**
     * @brief used inside the main function to break out of the sleep loop to
     * recalculate the next job
     */
    std::atomic_bool reloaded;

  private:
    /**
     * @brief used to hold the most recently ran job to speed up nextJob()
     *
     * @note
     * ?: Should this be a `std::size_t`?
     * ?: Is `iterator` an accurate name for this variable?
     */
    int iterator;

    /**
     * @brief vector of Jobs containing names of Tasks to sync and the time to
     * sync them
     */
    std::vector<Job> jobs;

    /**
     * @brief connection to log server
     */
    mirror::Logger* logger = mirror::Logger::getInstance();
};
} // namespace mirror::sync_scheduler
