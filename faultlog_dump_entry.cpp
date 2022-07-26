#include "faultlog_dump_entry.hpp"

#include <fmt/core.h>

#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{
namespace faultlog
{
using namespace phosphor::logging;

void Entry::delete_()
{
    // Delete Dump file from Permanent location
    try
    {
        std::filesystem::remove(file);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        // Log Error message and continue
        log<level::ERR>(
            fmt::format("Failed to delete dump file, errormsg({})", e.what())
                .c_str());
    }

    // Remove Dump entry D-bus object
    parentMap->erase(id);
}

} // namespace faultlog
} // namespace dump
} // namespace phosphor
