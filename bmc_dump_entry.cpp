#include "bmc_dump_entry.hpp"

#include "dump_manager.hpp"
#include "dump_offload.hpp"

#include <fmt/core.h>

#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{
namespace bmc
{
using namespace phosphor::logging;

void Entry::delete_()
{
    // Delete Dump file from Permanent location
    try
    {
        std::filesystem::remove_all(
            std::filesystem::path(path()).parent_path());
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        // Log Error message and continue
        log<level::ERR>(
            fmt::format("Failed to delete dump file({}), errormsg({})", path(),
                        e.what())
                .c_str());
    }

    // Remove Dump entry D-bus object
    phosphor::dump::Entry::delete_();
}

void Entry::initiateOffload(std::string uri)
{
    phosphor::dump::offload::requestOffload(path(), id, uri);
    offloaded(true);
}

} // namespace bmc
} // namespace dump
} // namespace phosphor
