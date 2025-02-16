/**
 * @file ProjectCatalogue.hpp
 * @author Cary Keesler
 * @brief
 */

#pragma once

// Standard Library Includes
#include <map>
#include <string>

// Project Includes
#include <mirror/sync_scheduler/SyncDetails.hpp>

namespace mirror::sync_scheduler
{
using ProjectCatalogue = std::map<std::string, SyncDetails>;
} // namespace mirror::sync_scheduler
