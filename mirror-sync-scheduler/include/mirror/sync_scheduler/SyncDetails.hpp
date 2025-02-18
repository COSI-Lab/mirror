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
enum class SyncMethod : std::uint8_t
{
    SCRIPT,
    RSYNC,
    UNSET,
};

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

    [[nodiscard]]
    auto get_sync_method() const -> SyncMethod
    {
        return m_SyncMethod;
    }

    [[nodiscard]]
    auto get_password_file() const -> nlohmann::json
    {
        return m_SyncConfig.value("password_file", "");
    }

    [[nodiscard]]
    auto get_sync_config() const -> nlohmann::json
    {
        return m_SyncConfig;
    }

  private: // Methods
    [[nodiscard]]
    static auto compose_rsync_command(
        const nlohmann::json& rsyncConfig,
        const std::string&    optionsKey
    ) -> std::string;

    [[nodiscard]]
    static auto generate_sync_config(const nlohmann::json& project)
        -> std::pair<SyncMethod, nlohmann::json>;

    [[nodiscard]]
    static auto generate_rsync_config(const nlohmann::json& project)
        -> std::pair<SyncMethod, nlohmann::json>;

    [[nodiscard]]
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
