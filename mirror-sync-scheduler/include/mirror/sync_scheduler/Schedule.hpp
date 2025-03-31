/**
 * @file Schedule.hpp
 * @author Cary Keesler
 * @brief
 */

#pragma once

// Standard Library Includes
#include <chrono>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>
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
  public: // Constructors
    explicit Schedule(const ProjectCatalogue& projects);

  public: // Methods
    [[nodiscard]]
    auto get_next_sync_batch() -> std::pair<
        std::chrono::time_point<std::chrono::system_clock>,
        std::set<std::string>>;

  private: // Methods
    auto build(const ProjectCatalogue& projects) -> void;
    auto verify(const ProjectCatalogue& projects) -> void;
    auto determine_sync_lcm(const ProjectCatalogue& projects) -> void;

  private: // Members
    std::vector<std::set<std::string>> m_SyncIntervals;
    std::size_t                        m_SyncLCM;
};
} // namespace mirror::sync_scheduler
