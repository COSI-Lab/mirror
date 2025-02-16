/**
 * @file SyncScheduler.hpp
 * @author Cary Keesler
 * @brief
 */

#pragma once

// Standard Library Includes
#include <map>
#include <string>

// Third Party Library Includes
#include <nlohmann/json.hpp>

// Project Includes
#include <mirror/sync_scheduler/ProjectCatalogue.hpp>
#include <mirror/sync_scheduler/Schedule.hpp>
#include <mirror/sync_scheduler/SyncDetails.hpp>

namespace mirror::sync_scheduler
{
class SyncScheduler
{
  public: // Constructors
    SyncScheduler();

  public: // Methods
    auto run() -> void;

  private:
    static auto load_mirrors_config() -> nlohmann::json;
    static auto generate_project_catalogue(const nlohmann::json& mirrors)
        -> ProjectCatalogue;

  private: // Members
    ProjectCatalogue m_ProjectCatalogue;
    Schedule         m_Schedule;
};
} // namespace mirror::sync_scheduler
