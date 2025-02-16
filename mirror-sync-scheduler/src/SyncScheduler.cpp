/**
 * @file SyncScheduler.cpp
 * @author Cary Keesler
 * @brief
 */

// Header Being Defined
#include <mirror/sync_scheduler/SyncScheduler.hpp>

// Standard Library Includes
#include <cerrno>
#include <cstdio>
#include <format>
#include <fstream>
#include <stdexcept>
// NOLINTNEXTLINE(*-deprecated-headers, llvm-include-order)
#include <string.h> // Required for `::strerror_r`. Not available in `cstring`
#include <string>
#include <utility>

// Third Party Library Includes
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

// Project Includes
#include <mirror/sync_scheduler/ProjectCatalogue.hpp>
#include <mirror/sync_scheduler/SyncDetails.hpp>

namespace mirror::sync_scheduler
{
SyncScheduler::SyncScheduler()
try
    : m_ProjectCatalogue(generate_project_catalogue(
          load_mirrors_config().value("mirrors", nlohmann::json {})
      )),
      m_Schedule(m_ProjectCatalogue)
{
    spdlog::info("Successfully generated schedule!");
}
catch (std::runtime_error& re)
{
    spdlog::error(re.what());
    throw re;
}

auto SyncScheduler::load_mirrors_config() -> nlohmann::json
{
    std::ifstream mirrorsConfigFile("configs/mirrors.json");

    if (!mirrorsConfigFile.good())
    {
        std::string errorMessage;
        errorMessage.resize(BUFSIZ);

        throw std::runtime_error(std::format(
            "Failed to load mirrors config! OS Error: {}",
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

    for (const auto& mirror : mirrors)
    {
        try
        {
            catalogue.emplace(
                std::make_pair(mirror.at("name"), SyncDetails(mirror))
            );
            spdlog::trace("Catalogued project {}", mirror.value("name", ""));
        }
        catch (static_project_exception& spe)
        {
            spdlog::warn(spe.what());
        }
        catch (std::runtime_error& re)
        {
            spdlog::error(re.what());
            throw re;
        }
    }

    return catalogue;
}

auto SyncScheduler::run() -> void
{
    // Actually do the thing
}
} // namespace mirror::sync_scheduler
