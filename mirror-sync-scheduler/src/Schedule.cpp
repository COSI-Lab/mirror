/**
 * @file Schedule.cpp
 * @author Cary Keesler
 * @brief
 */

// Header Being Defined
#include <mirror/sync_scheduler/Schedule.hpp>

// Standard Library Includes
#include <cstddef>
#include <format>
#include <map>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <string>

// Third Party Library Includes
#include <spdlog/spdlog.h>

// Project Includes
#include <mirror/sync_scheduler/ProjectCatalogue.hpp>

namespace mirror::sync_scheduler
{
Schedule::Schedule(const ProjectCatalogue& projects)
{
    build(projects);

    try
    {
        verify(projects);
    }
    catch (std::runtime_error& re)
    {
        spdlog::error(re.what());
        throw re;
    }

    spdlog::info("Successfully verified sync schedule!");
}

auto Schedule::sync_frequency_lcm(const ProjectCatalogue& projects)
    -> std::size_t
{
    std::size_t leastCommonMultiple = 1;

    for (const auto& [_, syncDetails] : projects)
    {
        leastCommonMultiple
            = std::lcm(leastCommonMultiple, syncDetails.get_syncs_per_day());
    }

    return leastCommonMultiple;
}

auto Schedule::build(const ProjectCatalogue& projects) -> void
{
    auto syncLCM = sync_frequency_lcm(projects);
    spdlog::trace(std::format("Sync LCM is {}", syncLCM));

    m_SyncIntervals.resize(syncLCM);

    for (auto [idx, interval] : std::ranges::enumerate_view(m_SyncIntervals))
    {
        for (const auto [jdx, project] : std::ranges::enumerate_view(projects))
        {
            const auto& [name, syncDetails] = project;

            if ((idx + 1) % (syncLCM / syncDetails.get_syncs_per_day()) == 0)
            {
                interval.emplace(name);
            }
        }
    }
}

auto Schedule::verify(const ProjectCatalogue& projects) -> void
{
    std::map<std::string, std::size_t> syncCounts;

    for (const auto& [idx, interval] :
         std::ranges::enumerate_view(m_SyncIntervals))
    {
        for (const auto& projectName : interval)
        {
            if (!syncCounts.contains(projectName))
            {
                syncCounts.emplace(projectName, 0);
            }

            syncCounts.at(projectName)++;
        }
    }

    for (const auto& [projectName, syncCount] : syncCounts)
    {
        if (projects.at(projectName).get_syncs_per_day() != syncCount)
        {
            throw std::runtime_error(std::format(
                "Failed to verify schedule! Project `{}` was scheduled to sync "
                "{} times; expected {}",
                projectName,
                syncCount,
                projects.at(projectName).get_syncs_per_day()
            ));
        }
    }
}
} // namespace mirror::sync_scheduler
