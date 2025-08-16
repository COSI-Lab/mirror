/**
 * @file SyncDetails.hpp
 * @author Cary Keesler
 * @brief
 */

#pragma once

// Standard Library Includes
#include <cstddef>
#include <cstdint>
#include <optional>
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
        return m_SyncsPerDay;
    }

    [[nodiscard]]
    auto get_password_file() const -> std::optional<std::filesystem::path>
    {
        return m_PasswordFile;
    }

    [[nodiscard]]
    auto get_commands() const -> std::vector<std::vector<std::string>>
    {
        return m_Commands;
    }

  private: // Methods
    [[nodiscard]]
    static auto compose_rsync_commands(const nlohmann::json& rsyncConfig)
        -> std::vector<std::vector<std::string>>;

    static auto handle_rsync_options_array(const nlohmann::json& optionsArray)
        -> std::vector<std::string>;

    auto handle_sync_config(const nlohmann::json& project) -> void;

    auto handle_rsync_config(const nlohmann::json& project) -> void;
    auto handle_script_config(const nlohmann::json& project) -> void;

  private: // Members
    std::size_t                           m_SyncsPerDay;
    std::optional<std::filesystem::path>  m_PasswordFile;
    std::vector<std::vector<std::string>> m_Commands;
};

struct static_project_exception : std::runtime_error
{
    using std::runtime_error::runtime_error;
};
} // namespace mirror::sync_scheduler
