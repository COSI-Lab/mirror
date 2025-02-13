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

// Third Party Library Includes
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

// Project Includes
#include <mirror/sync_scheduler/Project.hpp>

namespace mirror::sync_scheduler
{
Schedule::Schedule(const nlohmann::json& mirrors)
{
    if (mirrors.empty())
    {
        throw std::runtime_error("No mirrors found in config!");
    }

    for (const nlohmann::json& mirror : mirrors)
    {
        try
        {
            m_Projects.emplace_back(mirror);
        }
        catch (static_project_exception& spe)
        {
            spdlog::warn(spe.what());
        }
        catch (std::runtime_error& re)
        {
            spdlog::error(re.what());
            throw re;
        }
    }

    build();

    try
    {
        verify();
    }
    catch (std::runtime_error& re)
    {
        spdlog::error(re.what());
        throw re;
    }

    spdlog::info("Successfully verified sync schedule!");
}

auto Schedule::sync_frequency_lcm() -> std::size_t
{
    std::size_t leastCommonMultiple = 1;

    for (const auto& project : m_Projects)
    {
        leastCommonMultiple
            = std::lcm(leastCommonMultiple, project.get_syncs_per_day());
    }

    return leastCommonMultiple;
}

auto Schedule::build() -> void
{
    auto syncLCM = sync_frequency_lcm();
    spdlog::trace(std::format("Sync LCM is {}", syncLCM));

    m_SyncIntervals.resize(syncLCM);

    for (auto [idx, interval] : std::ranges::enumerate_view(m_SyncIntervals))
    {
        for (const auto [jdx, project] :
             std::ranges::enumerate_view(m_Projects))
        {
            if ((idx + 1) % (syncLCM / project.get_syncs_per_day()) == 0)
            {
                interval.emplace(jdx);
            }
        }
    }
}

auto Schedule::verify() -> void
{
    std::map<std::size_t, std::size_t> syncCounts;

    for (const auto& [idx, interval] :
         std::ranges::enumerate_view(m_SyncIntervals))
    {
        for (const auto projectIdx : interval)
        {
            if (!syncCounts.contains(projectIdx))
            {
                syncCounts.emplace(projectIdx, 0);
            }

            syncCounts.at(projectIdx)++;
        }
    }

    for (const auto [projectIdx, syncCount] : syncCounts)
    {
        if (m_Projects.at(projectIdx).get_syncs_per_day() != syncCount)
        {
            throw std::runtime_error(std::format(
                "Failed to verify schedule! Project `{}` was scheduled to sync "
                "{} times; expected {}",
                m_Projects.at(projectIdx).get_name(),
                syncCount,
                m_Projects.at(projectIdx).get_syncs_per_day()
            ));
        }
    }
}
} // namespace mirror::sync_scheduler
