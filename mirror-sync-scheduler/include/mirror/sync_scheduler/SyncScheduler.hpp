/**
 * @file SyncScheduler.hpp
 * @author Cary Keesler
 * @brief
 */

#pragma once

// Standard Library Includes
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Third Party Library Includes
#include <nlohmann/json.hpp>

// Project Includes
#include <mirror/sync_scheduler/JobManager.hpp>
#include <mirror/sync_scheduler/ProjectCatalogue.hpp>
#include <mirror/sync_scheduler/Schedule.hpp>

namespace mirror::sync_scheduler
{
class SyncScheduler
{
  public: // Constructors
    SyncScheduler();
    SyncScheduler(SyncScheduler&) = delete;
    SyncScheduler(SyncScheduler&&) = delete;
    auto operator=(SyncScheduler&) -> SyncScheduler = delete;
    auto operator=(SyncScheduler&&) -> SyncScheduler = delete;

  public: // Methods
    auto run() -> void;

  private: // Methods
  auto start_sync(const std::string& projectName) -> bool;
    
  private: // Static Methods
  static auto load_json_config(const std::filesystem::path& file)
      -> nlohmann::json;
  static auto generate_project_catalogue(const nlohmann::json& mirrors)
      -> ProjectCatalogue;

  private: // Members
    ProjectCatalogue m_ProjectCatalogue;
    Schedule         m_Schedule;
    JobManager       m_JobManager;
    bool             m_DryRun;
    std::jthread     m_ManualSyncThread;
};
} // namespace mirror::sync_scheduler
