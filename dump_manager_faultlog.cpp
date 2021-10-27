#include "config.h"

#include "dump_manager_faultlog.hpp"

#include "faultlog_dump_entry.hpp"

#include <fmt/core.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <sdbusplus/exception.hpp>
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
    log<level::INFO>("In dump_manager_fault.cpp createDump");

    // Currently we ignore the parameters.
    // TODO phosphor-debug-collector/issues/22: Check parameter values and
    // exit early if we don't receive the expected parameters
    if (params.empty())
    {
        log<level::INFO>("No additional parameters received");
    }
    else
    {
        log<level::INFO>("Got additional parameters");
    }

    // Get the originator id and type from params
    std::string oId;
    originatorTypes oType;

    phosphor::dump::extractOriginatorProperties(params, oId, oType);

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
        log<level::INFO>("faultLogFile is open");

        faultLogFile << "This is faultlog file #" << idString << " at "
                     << std::string(FAULTLOG_DUMP_PATH) + idString << std::endl;

        faultLogFile.close();
    }
    else
    {
        log<level::ERR>(fmt::format("Failed to open fault log file at {}, "
                                    "errno({}), strerror(\"{}\"), "
                                    "OBJECTPATH({}), ID({})",
                                    faultLogFilePath.c_str(), errno,
                                    strerror(errno), objPath.c_str(), id)
                            .c_str());
        elog<Open>(ErrnoOpen(errno), PathOpen(objPath.c_str()));
    }

    try
    {
        log<level::INFO>("dump_manager_faultlog.cpp: add faultlog entry");

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
                                   oId, oType, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        log<level::ERR>(fmt::format("Error in creating dump entry, "
                                    "errormsg({}), OBJECTPATH({}), ID({})",
                                    e.what(), objPath.c_str(), id)
                            .c_str());
        elog<InternalFailure>();
    }

    lastEntryId++;

    log<level::INFO>("End of dump_manager_faultlog.cpp createDump");
    return objPath.string();
}

} // namespace faultlog
} // namespace dump
} // namespace phosphor
