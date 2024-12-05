#include "bmc_dump_entry.hpp"

#include "dump_manager.hpp"
#include "dump_offload.hpp"
#include "dump_utils.hpp"

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
        std::filesystem::remove_all(file.parent_path());
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        // Log Error message and continue
        lg2::error("Failed to delete dump file, errormsg: {ERROR}", "ERROR", e);
    }

#ifdef LOG_PEL_ON_DUMP_ACTIONS
    auto bus = sdbusplus::bus::new_default();
    // Log PEL for dump delete
    phosphor::dump::createPELOnDumpActions(
        bus, file, "BMC Dump", std::to_string(id),
        "xyz.openbmc_project.Logging.Entry.Level.Informational",
        "xyz.openbmc_project.Dump.Error.Invalidate");
#endif
    // Remove Dump entry D-bus object
    phosphor::dump::Entry::delete_();
}

void Entry::initiateOffload(std::string uri)
{
    phosphor::dump::offload::requestOffload(file, id, uri);
    offloaded(true);
}

void Entry::updateFromFile(const std::filesystem::path& dumpPath)
{
    // Extract dump details from the file name
    auto dumpDetails = phosphor::dump::extractDumpDetails(dumpPath);
    if (!dumpDetails)
    {
        lg2::error("Failed to extract dump details from file name: {PATH}",
                   "PATH", dumpPath);
        throw std::logic_error("Invalid dump file name format");
    }

    auto [extractedId, extractedTimestamp, extractedSize] = *dumpDetails;

    if (id != extractedId)
    {
        lg2::error("Id({ID}) is not matching with id on filename"
                   "({ID_FILENAME})",
                   "ID", id, "ID_FILENAME", extractedId);
        throw std::logic_error("Invalid dump id in filename");
    }

    // Update the entry with extracted details
    startTime(extractedTimestamp);
    elapsed(extractedTimestamp);
    completedTime(extractedTimestamp);
    size(extractedSize);
    status(OperationStatus::Completed);
}

} // namespace bmc
} // namespace dump
} // namespace phosphor
