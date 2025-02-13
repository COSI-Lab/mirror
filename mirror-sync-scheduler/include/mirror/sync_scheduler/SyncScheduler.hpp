/**
 * @file SyncScheduler.hpp
 * @author Cary Keesler
 * @brief
 */

#pragma once

// Third Party Library Includes
#include <nlohmann/json.hpp>

// Project Includes
#include <mirror/sync_scheduler/Schedule.hpp>

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

  private: // Members
    Schedule m_Schedule;
};
} // namespace mirror::sync_scheduler
