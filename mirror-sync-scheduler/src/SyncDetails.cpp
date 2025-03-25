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

namespace mirror::sync_scheduler
{
SyncDetails::SyncDetails(const nlohmann::json& project)
    : m_SyncsPerDay(1)
{
    if (project.contains("static"))
    {
        throw static_project_exception(std::format(
            "Project '{}' uses a static sync. Project not created!",
            project.value("name", "")
        ));
    }

    const bool isRsyncOrScript
        = project.contains("rsync") || project.contains("script");

    if (!isRsyncOrScript)
    {
        throw std::runtime_error(std::format(
            "Project '{}' is missing a sync type!",
            project.value("name", "")
        ));
    }

    SyncDetails::handle_sync_config(project);
}

auto SyncDetails::compose_rsync_commands(const nlohmann::json& rsyncConfig)
    -> std::vector<std::string>
{
    std::vector<std::string> commands;

    using namespace std::string_view_literals;
    constexpr auto rsyncExecutable = "/usr/bin/rsync"sv;

    const std::string user = rsyncConfig.value("user", "");
    const std::string host = rsyncConfig.value("host", "");
    const std::string src  = rsyncConfig.value("src", "");
    const std::string dest = rsyncConfig.value("dest", "");

    for (const std::string options : rsyncConfig.at("options"))
    {
        if (options.empty())
        {
            throw std::runtime_error(std::format(
                "Project {} contains invalid rsync config options",
                rsyncConfig.value("name", "")
            ));
        }

        if (!user.empty())
        {
            commands.emplace_back(std::format(
                "{} {} {}@{}::{} {}",
                rsyncExecutable,
                options,
                user,
                host,
                src,
                dest
            ));
        }
        else
        {
            commands.emplace_back(std::format(
                "{} {} {}::{} {}",
                rsyncExecutable,
                options,
                host,
                src,
                dest
            ));
        }
    }

    return commands;
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
    m_SyncsPerDay = project.at("script").at("syncs_per_day").get<std::size_t>();

    std::string                    command = project.at("script").at("command");
    const std::vector<std::string> arguments
        = project.at("script").value("arguments", std::vector<std::string> {});

    for (const std::string& arg : arguments)
    {
        command = std::format("{} {}", command, arg);
    }

    m_Commands.emplace_back(command);
}
} // namespace mirror::sync_scheduler
