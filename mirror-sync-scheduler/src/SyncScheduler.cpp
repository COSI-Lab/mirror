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

// Third Party Library Includes
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

namespace mirror::sync_scheduler
{
SyncScheduler::SyncScheduler()
try
    : m_Schedule(load_mirrors_config().value("mirrors", nlohmann::json {}))
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

auto SyncScheduler::run() -> void { }
} // namespace mirror::sync_scheduler
