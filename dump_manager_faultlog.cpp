#include "config.h"

#include "dump_manager_faultlog.hpp"

#include "faultlog_dump_entry.hpp"

#include <fmt/core.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
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
    FaultDataType entryType = FaultDataType::Crashdump;
    std::string mainLogIdStr;
    bool badParam = false;

    log<level::INFO>("In dump_manager_fault.cpp createDump");

    // Currently we ignore the parameters.
    // TODO phosphor-debug-collector/issues/22: Check parameter values and
    // exit early if we don't receive the expected parameters
    if (params.empty())
    {
        log<level::INFO>("No additional parameters received");

        // Exit early
        return sdbusplus::message::object_path("");
    }
    else
    {
        log<level::INFO>("Got additional parameters");

        for (auto& [key, val] : params)
        {
            log<level::INFO>(fmt::format("key={}", key).c_str());
            std::visit(
                [&key, &entryType, &mainLogIdStr, &badParam](auto value) {
                    std::ostringstream oss;
                    oss << value;
                    log<level::INFO>(
                        fmt::format("value={}", oss.str()).c_str());

                    if constexpr (std::is_same_v<std::decay_t<decltype(value)>,
                                                 std::string>)
                    {
                        if (key.compare("Type") == 0)
                        {
                            if (value == "Crashdump")
                            {
                                entryType = FaultDataType::Crashdump;
                            }
                            else if (value == "CPER")
                            {
                                entryType = FaultDataType::CPER;
                            }
                            else
                            {
                                // Got unrecognized entry type
                                badParam = true;
                            }
                        }
                        else if (key.compare("MainLogId") == 0)
                        {
                            mainLogIdStr = value;
                        }
                    }
                },
                val);
        }
    }

    if (badParam)
    {
        // Exit early
        return sdbusplus::message::object_path("");
    }

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
                                   entryType, mainLogIdStr, *this)));
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

void Manager::init()
{
    // Base data types which we can handle by default
    using InterfaceVariant = typename sdbusplus::utility::dedup_variant_t<
        bool, uint8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t,
        size_t, ssize_t, double, std::string, sdbusplus::message::object_path>;

    using ChangedPropertiesType =
        std::vector<std::pair<std::string, InterfaceVariant>>;

    using ChangedInterfacesType =
        std::vector<std::pair<std::string, ChangedPropertiesType>>;

    crashdumpMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        "type='signal',interface='org.freedesktop.DBus.Properties',member='"
        "PropertiesChanged',path_namespace='/com/intel/crashdump'",

        [this](sdbusplus::message_t& msg) {
            if (msg.is_method_error())
            {
                log<level::ERR>("dump_manager_faultlog got crashdump error!");

                return;
            }

            log<level::INFO>("Got new crashdump notification!");

            std::string interface;
            std::string objpath;
            objpath = msg.get_path();

            ChangedPropertiesType changedProps;
            msg.read(interface, changedProps);

            if (interface == "com.intel.crashdump")
            {
                log<level::INFO>("interface is com.intel.crashdump");

                for (const auto& [changedProp, newValue] : changedProps)
                {
                    if (changedProp == "Log")
                    {
                        const auto* val = std::get_if<std::string>(&newValue);
                        if (val == nullptr)
                        {
                            return;
                        }

                        log<level::INFO>(fmt::format("val: {}", *val).c_str());

                        std::map<std::string,
                                 std::variant<std::string, uint64_t>>
                            crashdumpMap;

                        crashdumpMap.insert(std::pair<std::string, std::string>(
                            "Type", "Crashdump"));

                        crashdumpMap.insert(std::pair<std::string, std::string>(
                            "MainLogId",
                            std::filesystem::path(objpath).filename()));

                        createDump(crashdumpMap);
                    }
                }
            }
        });

    cperLogMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        "type='signal',path_namespace='/xyz/openbmc_project/external_storer/"
        "bios_bmc_smm_error_logger/CPER',"
        "interface='org.freedesktop.DBus.ObjectManager',member='"
        "InterfacesAdded'",

        [this](sdbusplus::message_t& msg) {
            if (msg.is_method_error())
            {
                log<level::INFO>(
                    "dump_manager_faultlog got cperLogMatch error!");
            }

            log<level::INFO>("Got new CPER Log notification!");

            sdbusplus::message::object_path newObjPath;
            std::string objpath;
            objpath = msg.get_path();

            ChangedPropertiesType changedProps;
            ChangedInterfacesType changedInterfaces;
            msg.read(newObjPath, changedInterfaces);

            log<level::INFO>(fmt::format("objpath: {}, newObjPath: {}", objpath,
                                         newObjPath.str)
                                 .c_str());

            for (const auto& [changedInterface, changedProps] :
                 changedInterfaces)
            {
                if (changedInterface == "xyz.openbmc_project.Common.FilePath")
                {
                    log<level::INFO>("changedInterface is "
                                     "xyz.openbmc_project.Common.FilePath");

                    for (const auto& [changedProp, newValue] : changedProps)
                    {
                        if (changedProp == "Path")
                        {
                            const auto* val =
                                std::get_if<std::string>(&newValue);
                            log<level::INFO>(
                                fmt::format("val: {}", *val).c_str());

                            std::string cperLogPath(CPER_LOG_PATH);
                            bool badPath = false;

                            // Check path length
                            if ((*val).size() <
                                cperLogPath.size() + CPER_LOG_ID_STRING_LEN)
                            {
                                badPath = true;
                                log<level::ERR>(
                                    fmt::format("CPER_LOG_ID_STRING_LEN: {}",
                                                CPER_LOG_ID_STRING_LEN)
                                        .c_str());
                            }
                            // Check path prefix
                            else if ((*val).compare(0, cperLogPath.size(),
                                                    cperLogPath) != 0)
                            {
                                badPath = true;
                                log<level::ERR>(
                                    fmt::format("prefix fail: {}", cperLogPath)
                                        .c_str());
                            }

                            if (badPath)
                            {
                                log<level::ERR>(
                                    fmt::format("Unexpected CPER log path: {}",
                                                *val)
                                        .c_str());
                            }
                            else
                            {
                                std::string cperId = val->substr(
                                    cperLogPath.size(), CPER_LOG_ID_STRING_LEN);
                                std::map<std::string,
                                         std::variant<std::string, uint64_t>>
                                    cperLogMap;
                                cperLogMap.insert(
                                    std::pair<std::string, std::string>(
                                        "Type", "CPER"));
                                cperLogMap.insert(
                                    std::pair<std::string, std::string>(
                                        "MainLogId", cperId));
                                createDump(cperLogMap);
                            }
                        }
                    }
                }
            }
        });
}

} // namespace faultlog
} // namespace dump
} // namespace phosphor
