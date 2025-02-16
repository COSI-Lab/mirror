/**
 * @file SyncDetails.hpp
 * @author Cary Keesler
 * @brief
 */

#pragma once

// Standard Library Includes
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Third Party Library Includes
#include <nlohmann/json.hpp>

namespace mirror::sync_scheduler
{
class SyncDetails
{
  public: // Constructors
    explicit SyncDetails(const nlohmann::json& project);

  public: // Methods
    [[nodiscard]]
    auto get_syncs_per_day() const -> std::size_t
    {
        return m_SyncConfig.at("syncs_per_day").get<std::size_t>();
    }

  private: // Enums
    enum class SyncMethod : std::uint8_t
    {
        SCRIPT,
        RSYNC,
        UNSET,
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
    SyncMethod     m_SyncMethod;
    nlohmann::json m_SyncConfig;
};

struct static_project_exception : std::runtime_error
{
    using std::runtime_error::runtime_error;
};
} // namespace mirror::sync_scheduler
