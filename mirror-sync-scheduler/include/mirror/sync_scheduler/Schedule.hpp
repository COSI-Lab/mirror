/**
 * @file Schedule.hpp
 * @author Cary Keesler
 * @brief
 */

#pragma once

// Standard Library Includes
#include <cstddef>
#include <map>
#include <set>
#include <vector>

// Third Party Library Includes
#include <nlohmann/json.hpp>

// Project Includes
#include <mirror/sync_scheduler/ProjectCatalogue.hpp>
#include <mirror/sync_scheduler/SyncDetails.hpp>

namespace mirror::sync_scheduler
{
class Schedule
{
  public:  // Constructors
    explicit Schedule(const ProjectCatalogue& projects);

  private: // Methods
    auto        build(const ProjectCatalogue& projects) -> void;
    auto        verify(const ProjectCatalogue& projects) -> void;
    static auto sync_frequency_lcm(const ProjectCatalogue& projects)
        -> std::size_t;

  private: // Members
    std::vector<std::set<std::string>> m_SyncIntervals;
};
} // namespace mirror::sync_scheduler
