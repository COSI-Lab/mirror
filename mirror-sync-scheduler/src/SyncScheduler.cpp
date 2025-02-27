/**
 * @file SyncScheduler.cpp
 * @author Cary Keesler
 * @brief
 */

// Header Being Defined
#include <mirror/sync_scheduler/SyncScheduler.hpp>

// Standard Library Includes
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>
// NOLINTNEXTLINE(*-deprecated-headers, llvm-include-order)
#include <string.h> // Required for `::strerror_r`. Not available in `cstring`
#include <thread>
#include <utility>

// Third Party Library Includes
#include <nlohmann/json_fwd.hpp>
// NOLINTNEXTLINE(misc-include-cleaner)
#include <spdlog/fmt/bundled/chrono.h> // Required to use a time variable in a log
// NOLINTNEXTLINE(misc-include-cleaner)
#include <spdlog/fmt/bundled/ranges.h> // Required to print a range in a log
#include <spdlog/spdlog.h>
#include <zmq.hpp>

// Project Includes
#include <mirror/sync_scheduler/ProjectCatalogue.hpp>
#include <mirror/sync_scheduler/SyncDetails.hpp>

namespace mirror::sync_scheduler
{
SyncScheduler::SyncScheduler()
try
    : m_ProjectCatalogue(
          generate_project_catalogue(load_json_config("configs/mirrors.json")
                                         .value("mirrors", nlohmann::json {}))
      ),
      m_Schedule(m_ProjectCatalogue),
      m_DryRun(false)
{
    auto config = load_json_config("configs/sync-scheduler.json");

    try
    {
        m_DryRun = config.at("dry_run");
    }
    catch (nlohmann::json::out_of_range& oor)
    {
        spdlog::warn("Key \"dry_run\" not set in config. Defaulting to false.");
    }

    if (m_DryRun)
    {
        spdlog::info("Dry run enabled. No syncing will occur.");
    }
}
catch (std::runtime_error& re)
{
    spdlog::error(re.what());
    throw re;
}

auto SyncScheduler::load_json_config(const std::filesystem::path& file)
    -> nlohmann::json
{
    std::ifstream mirrorsConfigFile(file);

    if (!mirrorsConfigFile.good())
    {
        static std::string errorMessage(BUFSIZ, '\0');

        throw std::runtime_error(std::format(
            "Failed to load config file {}! OS Error: {}",
            file.filename().string(),
            ::strerror_r(errno, errorMessage.data(), errorMessage.size())
        ));
    }

    return nlohmann::json::parse(mirrorsConfigFile);
}

auto SyncScheduler::generate_project_catalogue(const nlohmann::json& mirrors)
    -> ProjectCatalogue
{
    ProjectCatalogue catalogue;

    if (mirrors.empty())
    {
        throw std::runtime_error("No mirrors found in config!");
    }

    for (const auto& [name, mirrorDetails] : mirrors.items())
    {
        try
        {
            catalogue.emplace(std::make_pair(name, SyncDetails(mirrorDetails)));
            spdlog::trace(
                "Catalogued project {}",
                mirrorDetails.value("name", "")
            );
        }
        catch (static_project_exception& spe)
        {
            spdlog::trace(spe.what());
        }
        catch (std::runtime_error& re)
        {
            spdlog::error(re.what());
        }
    }

    return catalogue;
}

auto SyncScheduler::start_sync(const std::string& projectName) -> bool
{
    if (m_DryRun)
    {
        spdlog::info("Starting dry run sync for {}", projectName);
        return true;
    }

    if (m_JobManager.job_is_running(projectName))
    {
        spdlog::warn(
            "Sync for {} already running, not starting a new sync",
            projectName
        );
        return false;
    }

    spdlog::info("Attempting sync for {}", projectName);

    const auto& syncDetails     = m_ProjectCatalogue.at(projectName);
    const auto  syncConfig      = syncDetails.get_sync_config();
    bool        startSuccessful = false;

    if (syncDetails.get_sync_method() == SyncMethod::RSYNC)
    {
        startSuccessful = m_JobManager.start_job(
            projectName,
            syncConfig.at("primary"),
            syncDetails.get_password_file()
        );

        if (syncConfig.contains("secondary") && startSuccessful)
        {
            startSuccessful = m_JobManager.start_job(
                projectName,
                syncConfig.at("secondary"),
                syncDetails.get_password_file()
            );
        }

        if (syncConfig.contains("tertiary") && startSuccessful)
        {
            startSuccessful = m_JobManager.start_job(
                projectName,
                syncConfig.at("tertiary"),
                syncDetails.get_password_file()
            );
        }
    }
    else if (syncDetails.get_sync_method() == SyncMethod::SCRIPT)
    {
        startSuccessful = m_JobManager.start_job(
            projectName,
            syncConfig.at("command"),
            syncDetails.get_password_file()
        );
    }
    else
    {
        spdlog::error("Unrecognized sync method!");
        return false;
    }

    if (startSuccessful)
    {
        spdlog::info("Successfully started sync for {}", projectName);
    }
    else
    {
        spdlog::warn("Failed to start sync for {}", projectName);
    }
    return startSuccessful;
}

auto SyncScheduler::run() -> void
{
    spdlog::trace("Starting manual sync thread");
    m_ManualSyncThread = std::jthread(
        [this]() -> void
        {
            zmq::context_t socketContext {};
            zmq::socket_t  socket { socketContext, zmq::socket_type::rep };
            constexpr std::uint16_t MANUAL_SYNC_PORT = 9281;
            socket.bind(std::format("tcp://*:{}", MANUAL_SYNC_PORT));

            zmq::message_t syncRequest;

            while (true)
            {
                [[maybe_unused]]
                auto result
                    = socket.recv(syncRequest, zmq::recv_flags::none);
                const std::string projectName = syncRequest.to_string();

                spdlog::info("Manual sync requested for {}.", projectName);

                if (m_ProjectCatalogue.contains(projectName))
                {
                    if (start_sync(projectName))
                    {
                        socket.send(
                            zmq::message_t(std::format(
                                "SUCCESS: started sync for {}",
                                projectName
                            )),
                            zmq::send_flags::none
                        );
                        continue;
                    }

                    spdlog::error("Manual sync for {} failed", projectName);
                    socket.send(
                        zmq::message_t(std::format(
                            "FAILURE: Failed to start sync for {}",
                            projectName
                        )),
                        zmq::send_flags::none
                    );
                    continue;
                }

                spdlog::error("Project {} not found!", projectName);
                socket.send(
                    zmq::message_t(std::format(
                        "FAILURE: Project {} not found!",
                        projectName
                    )),
                    zmq::send_flags::none
                );
            }
        }
    );
    spdlog::info("Manual sync thread started!");

    spdlog::trace("Entering the main running loop");
    while (true)
    {
        const auto [timeOfNextSync, projectsToSync]
            = m_Schedule.get_next_sync_batch();

        spdlog::info(
            "Next sync scheduled for {:%m/%d @ %H:%M}. [{}]",
            timeOfNextSync,
            // NOLINTNEXTLINE(misc-include-cleaner)
            fmt::join(projectsToSync, ", ")
        );

        std::this_thread::sleep_until(timeOfNextSync);

        for (const auto& projectName : projectsToSync)
        {
            start_sync(projectName);
        }
    }

    spdlog::error("Leaving the main running loop! Program ending");
}
} // namespace mirror::sync_scheduler
