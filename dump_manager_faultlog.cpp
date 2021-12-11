#include "config.h"

#include "dump_manager_faultlog.hpp"

#include "dump_utils.hpp"
#include "bmc_dump_entry.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <fmt/core.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>

#include <iostream>
#include <fstream>
#include <filesystem>

namespace phosphor
{
namespace dump
{
namespace faultlog
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;


void Manager::notify(uint32_t dumpId, uint64_t size)
{
    log<level::INFO>(
            fmt::format(
                "dumpId={}, size={}", dumpId, size).c_str());
    
    /*
    // Get the timestamp
    std::time_t timeStamp = std::time(nullptr);

    // System dump can get created due to a fault in server
    // or by request from user. A system dump by fault is
    // first reported here, but for a user requested dump an
    // entry will be created first with invalid source id.
    // Since there can be only one system dump creation at a time,
    // if there is an entry with invalid sourceId update that.
    for (auto& entry : entries)
    {
        openpower::dump::system::Entry* sysEntry =
            dynamic_cast<openpower::dump::system::Entry*>(entry.second.get());
        if (sysEntry->sourceDumpId() == INVALID_SOURCE_ID)
        {
            sysEntry->update(timeStamp, size, dumpId);
            return;
        }
    }

    // Get the id
    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = std::filesystem::path(baseEntryPath) / idString;

    try
    {
        entries.insert(std::make_pair(
            id, std::make_unique<system::Entry>(
                    bus, objPath.c_str(), id, timeStamp, size, dumpId,
                    phosphor::dump::OperationStatus::Completed, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        log<level::ERR>(
            fmt::format(
                "Error in creating system dump entry, errormsg({}), "
                "OBJECTPATH({}), ID({}), TIMESTAMP({}),SIZE({}), SOURCEID({})",
                e.what(), objPath.c_str(), id, timeStamp, size, dumpId)
                .c_str());
        report<InternalFailure>();
        return;
    }
    lastEntryId++;
    return;
    */
}


sdbusplus::message::object_path
    Manager::createDump(phosphor::dump::DumpCreateParams params)
{
    /*constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
    constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
    constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";
    constexpr auto DIAG_MOD_TARGET = "obmc-host-crash@0.target";*/
    log<level::WARNING>("In dump_manager_fault.cpp createDump");

    if (!params.empty())
    {
        log<level::WARNING>("BMC dump accepts no additional parameters");
    }

    // Get the id
    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = std::filesystem::path(baseEntryPath) / idString;

    log<level::INFO>(fmt::format("objPath={}", objPath.c_str()).c_str());

    // std::vector<std::string> paths;
    // auto id = captureDump(Type::UserRequested, paths);

    // start capture dump code
    log<level::INFO>(fmt::format("faultLogFile={}", (std::string(FAULTLOG_DUMP_PATH) + idString).c_str()).c_str());

    std::ofstream faultLogFile;

    errno = 0;

    std::filesystem::create_directory(FAULTLOG_DUMP_PATH);
    faultLogFile.open((std::string(FAULTLOG_DUMP_PATH) + idString).c_str(), std::ofstream::out | std::fstream::trunc);

    auto openError = errno;
    log<level::INFO>(
        fmt::format("open errno is {}", openError)
            .c_str());

    if (faultLogFile.is_open())
    {
        log<level::INFO>("faultLogFile is open");
    }
    else
    {
        log<level::INFO>("faultLogFile is not open");
    }
    faultLogFile << "This is faultlog file #" << idString << " at " << std::string(FAULTLOG_DUMP_PATH) + idString << "\n";
    faultLogFile.close();
    /*
    pid_t pid = fork();

    if (pid == 0)
    {
        log<level::INFO>((std::string(FAULTLOG_DUMP_PATH) + idString).c_str());

        execl("/bin/mkdir", "mkdir", "-p", (std::string(FAULTLOG_DUMP_PATH) + idString).c_str(), nullptr);

        //execl("/bin/touch", "touch", std::string(FAULTLOG_DUMP_PATH) + idString, nullptr);
    }
    else if (pid > 0)
    {   
        auto rc = sd_event_add_child(eventLoop.get(), nullptr, pid,
                                     WEXITED | WSTOPPED, callback, nullptr);
        if (0 > rc)
        {
            // Failed to add to event loop
            log<level::ERR>(
                fmt::format(
                    "Error occurred during the sd_event_add_child call, rc({})",
                    rc)
                    .c_str());
            elog<InternalFailure>();
        }

        log<level::WARNING>("dump_manager_fault.cpp parent");
    }
    else
    {
        auto error = errno;
        log<level::ERR>(
            fmt::format("Error occurred during fork, errno({})", error)
                .c_str());
        elog<InternalFailure>();
    }
    */
    // end capture dump code

    try
    {
        std::time_t timeStamp = std::time(nullptr);
        entries.insert(std::make_pair(
            id, std::make_unique<bmc::Entry>(
                    bus, objPath.c_str(), id, timeStamp, 0, std::string(),
                    phosphor::dump::OperationStatus::InProgress, *this)));
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

    log<level::WARNING>("End of dump_manager_fault.cpp createDump");
    return objPath.string();

}

} // namespace faultlog
} // namespace dump
} // namespace phosphor
