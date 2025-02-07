/**
 * @file Schedule.cpp
 * @author Cary Keesler
 * @brief
 */

// Header Being Defined
#include <mirror/sync_scheduler/Schedule.hpp>

// Standard Library Includes
#include <stdexcept>

// Third Party Library Includes
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

// Project Includes
#include <mirror/sync_scheduler/Project.hpp>

namespace mirror::sync_scheduler
{
Schedule::Schedule(const nlohmann::json& mirrors)
    : m_Projects()
{
    if (mirrors.empty())
    {
        throw std::runtime_error("No mirrors found in config!");
    }

    for (const auto& mirror : mirrors)
    {
        try
        {
            m_Projects.emplace_back(Project(mirror));
        }
        catch (static_project_exception& e)
        {
            spdlog::warn(e.what());
        }
        catch (std::runtime_error& e)
        {
            spdlog::error(e.what());
        }
    }

    build();

    if (!verify())
    {
        throw std::runtime_error("Failed to verify sync schedule!");
    }
}

auto Schedule::build() -> void { }

auto Schedule::verify() -> bool
{
    return false;
}
} // namespace mirror::sync_scheduler
