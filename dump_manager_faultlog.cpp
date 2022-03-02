#include "config.h"

#include "dump_manager_faultlog.hpp"

#include "faultlog_dump_entry.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <fmt/core.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>

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

sdbusplus::message::object_path
    Manager::createDump(phosphor::dump::DumpCreateParams params)
{
    log<level::INFO>("In dump_manager_fault.cpp createDump");

    if (params.empty())
    {
        log<level::INFO>("No additional parameters received");
    }
    else
    {
        log<level::INFO>("Got additional parameters");
    }

    // Get the id
    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = std::filesystem::path(baseEntryPath) / idString;

    std::filesystem::path faultLogFilePath(std::string(FAULTLOG_DUMP_PATH) +
                                           idString);
    std::ofstream faultLogFile;

    errno = 0;

    std::filesystem::create_directory(FAULTLOG_DUMP_PATH);

    faultLogFile.open(faultLogFilePath,
                      std::ofstream::out | std::fstream::trunc);

    auto openError = errno;
    if (openError)
    {
        log<level::INFO>(fmt::format("open errno is {}", openError).c_str());
    }

    if (faultLogFile.is_open())
    {
        log<level::INFO>("faultLogFile is open");

        faultLogFile << "This is faultlog file #" << idString << " at "
                     << std::string(FAULTLOG_DUMP_PATH) + idString;

        faultLogFile.close();
    }
    else
    {
        log<level::INFO>("faultLogFile is not open");
    }

    try
    {
        log<level::INFO>("dump_manager_faultlog.cpp: add faultlog entry");

        uint64_t timestamp =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();

        entries.insert(std::make_pair(
            id,
            std::make_unique<faultlog::Entry>(
                bus, objPath.c_str(), id, timestamp,
                std::filesystem::file_size(faultLogFilePath), faultLogFilePath,
                phosphor::dump::OperationStatus::Completed, *this)));
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
