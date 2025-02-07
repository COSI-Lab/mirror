/**
 * @file Project.hpp
 * @author Cary Keesler
 * @brief
 */

#pragma once

// Standard Library Includes
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Third Party Library Includes
#include <nlohmann/json.hpp>

namespace mirror::sync_scheduler
{

class Project
{
  public:  // Constructors
    explicit Project(const nlohmann::json& project);

  public:  // Methods
  private: // Enums
    enum class SyncMethod : std::uint8_t
    {
        SCRIPT,
        RSYNC,
    };

  private: // Methods
    static auto compose_rsync_command(
        const nlohmann::json& rsyncConfig,
        const std::string&    optionsKey
    ) -> std::string;

    static auto generate_sync_config(const nlohmann::json& project)
        -> std::pair<SyncMethod, nlohmann::json>;

    static auto generate_rsync_config(const nlohmann::json& project)
        -> std::pair<SyncMethod, nlohmann::json>;

    static auto generate_script_config(const nlohmann::json& project)
        -> std::pair<SyncMethod, nlohmann::json>;

  private: // Members
    std::string    m_Name;
    SyncMethod     m_SyncMethod;
    nlohmann::json m_SyncConfig;
};

struct static_project_exception : std::runtime_error
{
    using std::runtime_error::runtime_error;
};
} // namespace mirror::sync_scheduler
