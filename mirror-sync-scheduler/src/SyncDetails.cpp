/**
 * @file SyncDetails.cpp
 * @author Cary Keesler
 * @brief
 */

// Header Being Defined
#include <mirror/sync_scheduler/SyncDetails.hpp>

// Standard Library Includes
#include <cstddef>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// Third Party Includes
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>
// NOLINTNEXTLINE(misc-include-cleaner) Required to print a range in a log
#include <spdlog/fmt/bundled/ranges.h>

namespace mirror::sync_scheduler
{
SyncDetails::SyncDetails(const nlohmann::json& project)
    : m_SyncsPerDay(1)
{
    if (project.contains("static"))
    {
        throw static_project_exception(
            std::format(
                "Project '{}' uses a static sync. Project not created!",
                project.value("name", "")
            )
        );
    }

    const bool isRsyncOrScript
        = project.contains("rsync") || project.contains("script");

    if (!isRsyncOrScript)
    {
        throw std::runtime_error(
            std::format(
                "Project '{}' is missing a sync type!",
                project.value("name", "")
            )
        );
    }

    SyncDetails::handle_sync_config(project);
}

auto SyncDetails::compose_rsync_commands(const nlohmann::json& rsyncConfig)
    -> std::vector<std::vector<std::string>>
{
    std::vector<std::vector<std::string>> commands;

    using namespace std::string_view_literals;
    constexpr auto RSYNC_EXECUTABLE = "/usr/bin/rsync"sv;

    const std::string user = rsyncConfig.value("user", "");
    const std::string host = rsyncConfig.value("host", "");
    const std::string src  = rsyncConfig.value("src", "");
    const std::string dest = rsyncConfig.value("dest", "");

    for (const nlohmann::json& options : rsyncConfig.at("options"))
    {
        if (options.empty())
        {
            throw std::runtime_error(
                std::format(
                    "Project {} contains invalid rsync config options",
                    rsyncConfig.value("name", "")
                )
            );
        }

        spdlog::trace("Options: {}", options.dump());

        if (options.is_array())
        {
            commands.emplace_back(
                SyncDetails::handle_rsync_options_array(options)
            );
        }
        else if (options.is_string())
        {
            if (commands.empty())
            {
                commands.resize(1);

                commands.front().emplace_back("/usr/bin/rsync");
            }

            commands.front().emplace_back(options);
        }
    }

    for (auto& command : commands)
    {
        if (!user.empty())
        {
            command.emplace_back(std::format("{}@{}::{}", user, host, src));
        }
        else
        {
            command.emplace_back(std::format("{}::{}", host, src));
        }

        command.emplace_back(dest);

        spdlog::trace("Sync Command: {{ {} }}", fmt::join(command, ", "));
    }

    return commands;
}

auto SyncDetails::handle_rsync_options_array(const nlohmann::json& optionsArray)
    -> std::vector<std::string>
{
    std::vector<std::string> toReturn = { "/usr/bin/rsync" };

    for (const std::string option : optionsArray)
    {
        toReturn.emplace_back(option);
    }

    return toReturn;
}

auto SyncDetails::handle_sync_config(const nlohmann::json& project) -> void
{
    if (project.contains("rsync"))
    {
        this->handle_rsync_config(project);
    }
    else if (project.contains("script"))
    {
        this->handle_script_config(project);
    }
    else
    {
        throw std::runtime_error("Unknown project type");
    }
}

auto SyncDetails::handle_rsync_config(const nlohmann::json& project) -> void
{
    const auto& rsyncConfig = project.at("rsync");

    m_SyncsPerDay = rsyncConfig.at("syncs_per_day").get<std::size_t>();

    m_Commands = SyncDetails::compose_rsync_commands(rsyncConfig);

    if (rsyncConfig.contains("password_file"))
    {
        m_PasswordFile = rsyncConfig.at("password_file");
    }
}

auto SyncDetails::handle_script_config(const nlohmann::json& project) -> void
{
    const auto& scriptBlock = project.at("script");

    m_SyncsPerDay = scriptBlock.at("syncs_per_day").get<std::size_t>();

    std::vector<std::string> toReturn = { "/usr/bin/bash", "-c" };

    toReturn.emplace_back(scriptBlock.at("command"));

    toReturn.back() = "\"";

    for (const auto& arg : scriptBlock.at("arguments"))
    {
        toReturn.back() = std::format("{} {}", toReturn.back(), arg.dump());
    }

    toReturn.back() = "\"";

    spdlog::trace("Sync Command: {{ {} }}", fmt::join(toReturn, ", "));

    m_Commands.emplace_back(toReturn);
}
} // namespace mirror::sync_scheduler
