#include "bmc_dump_entry.hpp"

#include "dump_manager.hpp"
#include "dump_offload.hpp"

#include <phosphor-logging/lg2.hpp>

namespace phosphor
{
namespace dump
{
namespace bmc
{

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
        lg2::error(
            "Failed to delete dump file: {DUMP_FILE}, errormsg: {ERROR_MSG}",
            "DUMP_FILE", path(), "ERROR_MSG", e.what());
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
