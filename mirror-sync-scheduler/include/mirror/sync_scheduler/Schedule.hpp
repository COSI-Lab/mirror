/**
 * @file Schedule.hpp
 * @author Cary Keesler
 * @brief
 */

#pragma once

// Standard Library Includes
#include <cstddef>
#include <set>
#include <vector>

// Third Party Library Includes
#include <nlohmann/json.hpp>

// Project Includes
#include <mirror/sync_scheduler/Project.hpp>

namespace mirror::sync_scheduler
{
class Schedule
{
  public:  // Constructors
    explicit Schedule(const nlohmann::json& mirrors);

  private: // Methods
    auto build() -> void;
    auto verify() -> void;
    auto sync_frequency_lcm() -> std::size_t;

  private: // Members
    std::vector<Project>               m_Projects;
    std::vector<std::set<std::size_t>> m_SyncIntervals;
};
} // namespace mirror::sync_scheduler
