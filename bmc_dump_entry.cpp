#include "bmc_dump_entry.hpp"

#include "dump_manager.hpp"

#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{
namespace BMC
{
using namespace phosphor::logging;

void Entry::delete_()
{
    // Delete Dump file from Permanent location
    try
    {
        fs::remove_all(file.parent_path());
    }
    catch (fs::filesystem_error& e)
    {
        // Log Error message and continue
        log<level::ERR>(e.what());
    }

    // Remove Dump entry D-bus object
    parent.erase(id);
}

} // namespace
} // namespace dump
} // namespace phosphor
