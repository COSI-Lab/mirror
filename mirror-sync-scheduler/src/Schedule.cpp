/**
 * @file Schedule.cpp
 * @author Cary Keesler
 * @brief
 */

// Header Being Defined
#include <mirror/sync_scheduler/Schedule.hpp>

// Standard Library Includes
#include <chrono>
#include <cstddef>
#include <format>
#include <map>
#include <numeric>
#include <ranges>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

// Third Party Library Includes
#include <spdlog/spdlog.h>

// Project Includes
#include <mirror/sync_scheduler/ProjectCatalogue.hpp>

namespace mirror::sync_scheduler
{
Schedule::Schedule(const ProjectCatalogue& projects)
    : m_SyncLCM(1)
{
    this->build(projects);

    try
    {
        this->verify(projects);
    }
    catch (std::runtime_error& re)
    {
        spdlog::error(re.what());
    }

    spdlog::info("Successfully built and verified sync schedule!");
}

auto Schedule::determine_sync_lcm(const ProjectCatalogue& projects) -> void
{
    for ([[maybe_unused]] const auto& [projectName, syncDetails] : projects)
    {
        m_SyncLCM = std::lcm(m_SyncLCM, syncDetails.get_syncs_per_day());
    }
}

auto Schedule::build(const ProjectCatalogue& projects) -> void
{
    determine_sync_lcm(projects);
    spdlog::trace("Sync LCM is {}", m_SyncLCM);

    m_SyncIntervals.resize(m_SyncLCM);

    spdlog::debug("Building sync schedule");
    for (auto [idx, interval] : std::ranges::enumerate_view(m_SyncIntervals))
    {
        for (const auto& [name, syncDetails] : projects)
        {
            if ((idx + 1) % (m_SyncLCM / syncDetails.get_syncs_per_day()) == 0)
            {
                interval.emplace(name);
            }
        }
    }
    spdlog::debug("Successfully built sync schedule!");
}

auto Schedule::verify(const ProjectCatalogue& projects) -> void
{
    std::map<std::string, std::size_t> syncCounts;

    spdlog::debug("Verifying sync schedule");
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

    spdlog::debug("Successfully verified sync schedule");
}

auto Schedule::get_next_sync_batch() -> std::pair<
    std::chrono::time_point<std::chrono::system_clock>,
    std::set<std::string>>
{
    // NOLINTBEGIN(misc-include-cleaner)
    auto intervalLength
        = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::days(1))
        / m_SyncLCM;
    const auto now      = std::chrono::system_clock::now();
    const auto midnight = std::chrono::floor<std::chrono::days>(now);
    std::chrono::time_point<std::chrono::system_clock> nextSync
        = midnight + intervalLength;
    // NOLINTEND(misc-include-cleaner)

    spdlog::trace(
        "Determining next batch of syncs. Current time in seconds since "
        "midnight: {}",
        std::chrono::duration_cast<std::chrono::seconds>(now - midnight).count()
    );
    for ([[maybe_unused]] const auto [idx, interval] :
         m_SyncIntervals | std::views::enumerate)
    {
        if (nextSync > now)
        {
            return std::make_pair(nextSync, m_SyncIntervals.at(idx));
        }

        spdlog::trace(
            "Possible next sync: {}",
            std::chrono::duration_cast<std::chrono::seconds>(
                nextSync - midnight
            )
                .count()
        );

        nextSync += intervalLength;
    }

    spdlog::error("Failed to determine next sync");
    return {};
}
} // namespace mirror::sync_scheduler
