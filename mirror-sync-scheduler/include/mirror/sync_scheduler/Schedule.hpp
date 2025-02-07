/**
 * @file Schedule.hpp
 * @author Cary Keesler
 * @brief
 */

#pragma once

// Standard Library Includes
#include <array>
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

  public:  // Methods
    auto build() -> void;

  private: // Methods
    auto verify() -> bool;

  private: // Members
    std::vector<Project>                     m_Projects;
    std::array<std::vector<std::size_t>, 24> m_HoursToSync;
};
} // namespace mirror::sync_scheduler
