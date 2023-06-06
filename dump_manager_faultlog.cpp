#include "config.h"

#include "dump_manager_faultlog.hpp"

#include "dump_utils.hpp"
#include "faultlog_dump_entry.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Common/File/error.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace phosphor
{
namespace dump
{
namespace faultlog
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace sdbusplus::xyz::openbmc_project::Common::File::Error;
using ErrnoOpen = xyz::openbmc_project::Common::File::Open::ERRNO;
using PathOpen = xyz::openbmc_project::Common::File::Open::PATH;

sdbusplus::message::object_path
    Manager::createDump(phosphor::dump::DumpCreateParams params)
{
    lg2::info("In dump_manager_fault.cpp createDump");

    // Currently we ignore the parameters.
    // TODO phosphor-debug-collector/issues/22: Check parameter values and
    // exit early if we don't receive the expected parameters
    if (params.empty())
    {
        lg2::info("No additional parameters received");
    }
    else
    {
        lg2::info("Got additional parameters");
    }

    // Get the originator id and type from params
    std::string originatorId;
    originatorTypes originatorType;

    phosphor::dump::extractOriginatorProperties(params, originatorId,
                                                originatorType);

    // Get the id
    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = std::filesystem::path(baseEntryPath) / idString;

    std::filesystem::path faultLogFilePath(std::string(FAULTLOG_DUMP_PATH) +
                                           idString);
    std::ofstream faultLogFile;

    errno = 0;

    faultLogFile.open(faultLogFilePath,
                      std::ofstream::out | std::fstream::trunc);

    if (faultLogFile.is_open())
    {
        lg2::info("faultLogFile is open");

        faultLogFile << "This is faultlog file #" << idString << " at "
                     << std::string(FAULTLOG_DUMP_PATH) + idString << std::endl;

        faultLogFile.close();
    }
    else
    {
        lg2::error(
            "Failed to open fault log file at {FILE_PATH}, errno: {ERRNO}, "
            "strerror: {STRERROR}, OBJECTPATH: {OBJECT_PATH}, ID: {ID}",
            "FILE_PATH", faultLogFilePath.c_str(), "ERRNO", errno, "STRERROR",
            strerror(errno), "OBJECT_PATH", objPath.c_str(), "ID", id);
        elog<Open>(ErrnoOpen(errno), PathOpen(objPath.c_str()));
    }

    try
    {
        lg2::info("dump_manager_faultlog.cpp: add faultlog entry");

        uint64_t timestamp =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();

        entries.insert(
            std::make_pair(id, std::make_unique<faultlog::Entry>(
                                   bus, objPath.c_str(), id, timestamp,
                                   std::filesystem::file_size(faultLogFilePath),
                                   faultLogFilePath,
                                   phosphor::dump::OperationStatus::Completed,
                                   originatorId, originatorType, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        lg2::error("Error in creating dump entry, errormsg: {ERROR_MSG}, "
                   "OBJECTPATH: {OBJECT_PATH}, ID: {ID}",
                   "ERROR_MSG", e, "OBJECT_PATH", objPath.c_str(), "ID", id);
        elog<InternalFailure>();
    }

    lastEntryId++;

    lg2::info("End of dump_manager_faultlog.cpp createDump");
    return objPath.string();
}

} // namespace faultlog
} // namespace dump
} // namespace phosphor
