/**
 * @file Project.cpp
 * @author Cary Keesler
 * @brief
 */

// Header Being Defined
#include <mirror/sync_scheduler/Project.hpp>

// Standard Library Includes
#include <format>
#include <vector>

// Third Party Includes
#include <nlohmann/json.hpp>

namespace mirror::sync_scheduler
{
Project::Project(const nlohmann::json& project)
{
    if (project.contains("static"))
    {
        throw static_project_exception(std::format(
            "Project '{}' uses a static sync. Project not created",
            project.value("name", "")
        ));
    }

    const bool isRsyncOrScript
        = project.contains("rsync") || project.contains("script");

    if (!isRsyncOrScript)
    {
        throw std::runtime_error(std::format(
            "Project '{}' is missing a sync type",
            project.value("name", "")
        ));
    }

    m_Name = project.value("name", "");

    std::tie(m_SyncMethod, m_SyncConfig) = generate_sync_config(project);
}

auto Project::compose_rsync_command(
    const nlohmann::json& rsyncConfig,
    const std::string&    optionsKey = "options"
) -> std::string
{
    std::string command
        = std::format("/usr/bin/rsync %s", rsyncConfig.value(optionsKey, ""));

    const std::string user = rsyncConfig.value("user", "");
    const std::string host = rsyncConfig.value("host", "");
    const std::string src  = rsyncConfig.value("src", "");
    const std::string dest = rsyncConfig.value("dest", "");

    if (!user.empty())
    {
        command
            = std::format("{} {}@{}::{} {}", command, user, host, src, dest);
    }
    else
    {
        command = std::format("{} {}::{} {}", command, host, src, dest);
    }

    return command;
}

auto Project::generate_sync_config(const nlohmann::json& project)
    -> std::pair<Project::SyncMethod, nlohmann::json>
{
    return project.contains("rsync") ? generate_rsync_config(project)
                                     : generate_script_config(project);
}

auto Project::generate_rsync_config(const nlohmann::json& project)
    -> std::pair<Project::SyncMethod, nlohmann::json>
{
    nlohmann::json config;

    config.emplace("syncs_per_day", project.at("rsync").at("syncs_per_day"));
    config.emplace("primary", compose_rsync_command(project.at("rsync")));

    if (project.at("rsync").contains("second"))
    {
        config.emplace(
            "secondary",
            compose_rsync_command(project.at("rsync"), "second")
        );
    }
    if (project.at("rsync").contains("third"))
    {
        config.emplace(
            "tertiary",
            compose_rsync_command(project.at("rsync"), "third")
        );
    }

    if (project.contains("password_file"))
    {
        config.emplace("password_file", project.at("password_file"));
    }

    return std::make_pair(Project::SyncMethod::RSYNC, config);
}

auto Project::generate_script_config(const nlohmann::json& project)
    -> std::pair<Project::SyncMethod, nlohmann::json>
{
    nlohmann::json config;

    config.emplace("syncs_per_day", project.at("script").at("syncs_per_day"));

    std::string              command = project.at("script").at("command");
    std::vector<std::string> arguments
        = project.at("script").value("arguments", std::vector<std::string> {});

    for (const std::string& arg : arguments)
    {
        command = std::format("%s %s", command, arg);
    }

    config.emplace("command", command);

    return std::make_pair(Project::SyncMethod::SCRIPT, config);
}
} // namespace mirror::sync_scheduler
