#include "config.h"

#include "dump_manager_faultlog.hpp"

#include "dump_utils.hpp"
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

using InterfaceVariant = typename sdbusplus::utility::dedup_variant_t<
    bool, uint8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t,
    size_t, ssize_t, double, std::string, sdbusplus::message::object_path,
    std::map<std::string, std::string>, std::vector<std::string>>;

using ChangedPropertiesType =
    std::vector<std::pair<std::string, InterfaceVariant>>;

using ChangedInterfacesType =
    std::vector<std::pair<std::string, ChangedPropertiesType>>;

sdbusplus::message::object_path
    Manager::createDump(phosphor::dump::DumpCreateParams params)
{
    FaultDataType entryType = FaultDataType::Crashdump;
    std::string primaryLogIdStr;

    log<level::INFO>("In dump_manager_faultlog.cpp createDump");

    getAndCheckCreateDumpParams(params, entryType, primaryLogIdStr);

    // To stay within the limit of MAX_NUM_FAULT_LOG_ENTRIES we need to remove
    // an entry from the fault log map to make room for creating a new entry
    if (entries.size() == MAX_NUM_FAULT_LOG_ENTRIES)
    {
        // Save the earliest fault log entry to a saved entries map (if
        // it qualifies to be saved), and remove it from the main fault
        // log entries map.
        saveEarliestEntry();
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

    log<level::INFO>(
        fmt::format("next entry id: {}, entries.size(): {}", id, entries.size())
            .c_str());

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

        entries.insert(std::make_pair(
            id,
            std::make_unique<faultlog::Entry>(
                bus, objPath.c_str(), id, generateTimestamp(),
                std::filesystem::file_size(faultLogFilePath), faultLogFilePath,
                phosphor::dump::OperationStatus::Completed, originatorId,
                originatorType, entryType, primaryLogIdStr, *this, &entries)));
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

void Manager::deleteAll()
{
    log<level::INFO>("In dump_manager_faultlog.hpp deleteAll");

    phosphor::dump::Manager::deleteAll();

    auto iter = savedCperLogEntries.begin();
    while (iter != savedCperLogEntries.end())
    {
        auto& entry = iter->second;
        entry->delete_();
        ++iter;
    }

    iter = savedCrashdumpEntries.begin();
    while (iter != savedCrashdumpEntries.end())
    {
        auto& entry = iter->second;
        entry->delete_();
        ++iter;
    }
}

void Manager::registerFaultLogMatches()
{
    log<level::INFO>("dump_manager_faultlog registerFaultLogMatches");

    registerCrashdumpMatch();
    registerCperLogMatch();
}

void Manager::registerCrashdumpMatch()
{
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
                            log<level::ERR>("Couldn't get Log property");
                            return;
                        }

                        log<level::INFO>(fmt::format("Log: {}", *val).c_str());

                        std::map<std::string,
                                 std::variant<std::string, uint64_t>>
                            crashdumpMap;

                        crashdumpMap.insert(std::pair<std::string, std::string>(
                            "Type", "Crashdump"));

                        crashdumpMap.insert(std::pair<std::string, std::string>(
                            "PrimaryLogId",
                            std::filesystem::path(objpath).filename()));

                        createDump(crashdumpMap);
                    }
                }
            }
        });
}

void Manager::registerCperLogMatch()
{
    cperLogMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        "type='signal',sender='xyz.openbmc_project.Logging',"
        "path_namespace='/xyz/openbmc_project/logging',"
        "interface='org.freedesktop.DBus.ObjectManager',member='"
        "InterfacesAdded'",
        [this](sdbusplus::message_t& msg) {
            if (msg.is_method_error())
            {
                log<level::INFO>(
                    "dump_manager_faultlog got cperLogMatch error!");
            }

            sdbusplus::message::object_path newObjPath;

            ChangedPropertiesType changedProps;
            ChangedInterfacesType changedInterfaces;
            msg.read(newObjPath, changedInterfaces);

            log<level::INFO>(
                fmt::format("newObjPath: {}", newObjPath.str).c_str());

            for (const auto& [changedInterface, changedProps] :
                 changedInterfaces)
            {
                if (changedInterface == "xyz.openbmc_project.Logging.Entry")
                {
                    log<level::INFO>("changedInterface: "
                                     "xyz.openbmc_project.Logging.Entry");

                    bool messageFound = false;
                    bool additionalDataFound = false;
                    const std::string* messageVal = nullptr;
                    const std::vector<std::string>* additionalDataKeyValPairs =
                        nullptr;

                    for (const auto& [changedProp, newValue] : changedProps)
                    {
                        if (changedProp == "Message")
                        {
                            messageFound = true;

                            messageVal = std::get_if<std::string>(&newValue);

                            if (messageVal == nullptr)
                            {
                                log<level::ERR>(
                                    "Couldn't get Message property");
                                return;
                            }

                            if (*messageVal ==
                                "xyz.openbmc_project.Common.Error.LogDataAdded")
                            {
                                log<level::INFO>(
                                    fmt::format("Message: {}", *messageVal)
                                        .c_str());
                            }
                            else
                            {
                                log<level::WARNING>(
                                    fmt::format("Got unrecognized message: {}",
                                                *messageVal)
                                        .c_str());
                                return;
                            }
                        }
                        else if (changedProp == "AdditionalData")
                        {
                            additionalDataFound = true;
                            additionalDataKeyValPairs =
                                std::get_if<std::vector<std::string>>(
                                    &newValue);

                            if (additionalDataKeyValPairs == nullptr)
                            {
                                log<level::ERR>(
                                    "Couldn't get AdditionalData property");
                                return;
                            }
                        }

                        if (messageFound && additionalDataFound)
                        {
                            break;
                        }
                    }

                    if (!messageFound || messageVal == nullptr)
                    {
                        log<level::WARNING>("Message not found");
                        return;
                    }

                    if (!additionalDataFound ||
                        additionalDataKeyValPairs == nullptr)
                    {
                        log<level::WARNING>("AdditionalData not found");
                        return;
                    }

                    bool originFound = false;
                    bool primaryLogIdFound = false;
                    bool typeFound = false;

                    std::map<std::string, std::variant<std::string, uint64_t>>
                        cperLogMap;

                    for (const auto& keyValStr : *additionalDataKeyValPairs)
                    {
                        log<level::INFO>(
                            fmt::format("keyValStr: {}", keyValStr).c_str());

                        constexpr char const* originEquals = "Origin=";
                        constexpr char const* primaryLogIdEquals =
                            "PrimaryLogId=";
                        constexpr char const* typeEquals = "Type=";
                        constexpr char const* additionalTypeNameEquals =
                            "AdditionalTypeName=";
                        constexpr std::size_t lenOriginEquals =
                            std::char_traits<char>::length(originEquals);
                        constexpr std::size_t lenPrimaryLogIdEquals =
                            std::char_traits<char>::length(primaryLogIdEquals);
                        constexpr std::size_t lenTypeEquals =
                            std::char_traits<char>::length(typeEquals);
                        constexpr std::size_t lenAdditionalTypeNameEquals =
                            std::char_traits<char>::length(
                                additionalTypeNameEquals);

                        if (keyValStr.compare(0, lenOriginEquals,
                                              originEquals) == 0)
                        {
                            originFound = true;
                            std::string origin =
                                keyValStr.substr(lenOriginEquals);
                            if (origin.compare(HOST_CPER_LOGGER_BUSNAME) != 0)
                            {
                                log<level::WARNING>(
                                    fmt::format(
                                        "AdditionalData unexpected origin: {}",
                                        origin)
                                        .c_str());
                                return;
                            }
                        }
                        else if (keyValStr.compare(0, lenPrimaryLogIdEquals,
                                                   primaryLogIdEquals) == 0)
                        {
                            primaryLogIdFound = true;
                            std::string primaryLogId =
                                keyValStr.substr(lenPrimaryLogIdEquals);
                            cperLogMap.insert(
                                std::pair<std::string, std::string>(
                                    "PrimaryLogId", primaryLogId));
                        }
                        else if (keyValStr.compare(0, lenTypeEquals,
                                                   typeEquals) == 0)
                        {
                            typeFound = true;
                            std::string type = keyValStr.substr(lenTypeEquals);
                            if (type.compare("xyz.openbmc_project.Dump.Entry."
                                             "FaultLog.CPER") == 0)
                            {
                                log<level::INFO>(
                                    "Got notification of new CPER Log!");
                                cperLogMap.insert(
                                    std::pair<std::string, std::string>(
                                        "Type", "CPER"));
                            }
                            else
                            {
                                log<level::WARNING>(
                                    fmt::format(
                                        "AdditionalData unexpected type: {}",
                                        type)
                                        .c_str());
                                return;
                            }
                        }
                        else if (keyValStr.compare(
                                     0, lenAdditionalTypeNameEquals,
                                     additionalTypeNameEquals) == 0)
                        {
                            std::string additionalTypeName =
                                keyValStr.substr(lenAdditionalTypeNameEquals);
                            cperLogMap.insert(
                                std::pair<std::string, std::string>(
                                    "AdditionalTypeName", additionalTypeName));
                        }
                    }

                    if ((originFound == false) ||
                        (primaryLogIdFound == false) || (typeFound == false))
                    {
                        log<level::WARNING>("AdditionalData incomplete");
                        return;
                    }

                    createDump(cperLogMap);

                    break;
                }
            }
        });

    cperLogMatchOld = std::make_unique<sdbusplus::bus::match_t>(
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

            ChangedPropertiesType changedProps;
            ChangedInterfacesType changedInterfaces;
            msg.read(newObjPath, changedInterfaces);

            log<level::INFO>(
                fmt::format("newObjPath: {}", newObjPath.str).c_str());

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

                            if (val == nullptr)
                            {
                                log<level::ERR>("Couldn't get Path property");
                                return;
                            }

                            log<level::INFO>(
                                fmt::format("Path: {}", *val).c_str());

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
                                        "PrimaryLogId", cperId));
                                createDump(cperLogMap);
                            }
                        }
                    }
                }
            }
        });
}

void Manager::getAndCheckCreateDumpParams(
    const phosphor::dump::DumpCreateParams& params, FaultDataType& entryType,
    std::string& primaryLogIdStr)
{
    using InvalidArgument =
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
    using Argument = xyz::openbmc_project::Common::InvalidArgument;
    std::string value;

    auto iter = params.find("Type");
    if (iter == params.end())
    {
        log<level::ERR>("Required argument Type is missing");
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("TYPE"),
                              Argument::ARGUMENT_VALUE("MISSING"));
    }
    else
    {
        try
        {
            value = std::get<std::string>(iter->second);
        }
        catch (const std::bad_variant_access& e)
        {
            // Exception will be raised if the input is not string
            log<level::ERR>(
                fmt::format("An invalid Type string is passed errormsg({})",
                            e.what())
                    .c_str());
            elog<InvalidArgument>(Argument::ARGUMENT_NAME("TYPE"),
                                  Argument::ARGUMENT_VALUE("INVALID INPUT"));
        }

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
            log<level::ERR>("Unexpected entry type, not handled");
            elog<InvalidArgument>(Argument::ARGUMENT_NAME("TYPE"),
                                  Argument::ARGUMENT_VALUE("UNEXPECTED TYPE"));
        }
    }

    iter = params.find("PrimaryLogId");
    if (iter == params.end())
    {
        log<level::ERR>("Required argument PrimaryLogId is missing");
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("PRIMARYLOGID"),
                              Argument::ARGUMENT_VALUE("MISSING"));
    }
    else
    {
        try
        {
            value = std::get<std::string>(iter->second);
        }
        catch (const std::bad_variant_access& e)
        {
            // Exception will be raised if the input is not string
            log<level::ERR>(
                fmt::format(
                    "An invalid PrimaryLogId string is passed errormsg({})",
                    e.what())
                    .c_str());
            elog<InvalidArgument>(Argument::ARGUMENT_NAME("PRIMARYLOGID"),
                                  Argument::ARGUMENT_VALUE("INVALID INPUT"));
        }

        if (value.empty())
        {
            log<level::ERR>("Got empty PrimaryLogId string");
            elog<InvalidArgument>(Argument::ARGUMENT_NAME("PRIMARYLOGID"),
                                  Argument::ARGUMENT_VALUE("EMPTY STRING"));
        }

        primaryLogIdStr = value;
    }
}

uint64_t Manager::generateTimestamp()
{
    uint64_t timestamp =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    // Ensure unique and increasing timestamps. In case the current
    // generated timestamp is smaller or equal to the last generated
    // timestamp, make the current timestamp the previous timestamp
    // incremented by 1 microsecond.
    if (timestamp <= prevTimestamp)
    {
        timestamp = prevTimestamp + 1;
    }

    prevTimestamp = timestamp;
    return timestamp;
}

void Manager::saveEarliestEntry()
{
    auto earliestEntry = entries.begin();
    uint32_t earliestEntryId = earliestEntry->first;
    auto earliestEntryPtr = (earliestEntry->second).get();
    FaultDataType earliestEntryType =
        dynamic_cast<faultlog::Entry*>(earliestEntryPtr)->type();

    size_t maxNumSavedEntries = 0;
    std::map<uint32_t, std::unique_ptr<phosphor::dump::Entry>>* savedEntries;

    switch (earliestEntryType)
    {
        case FaultDataType::CPER:
            maxNumSavedEntries = MAX_NUM_SAVED_CPER_LOG_ENTRIES;
            savedEntries = &savedCperLogEntries;
            break;
        case FaultDataType::Crashdump:
            maxNumSavedEntries = MAX_NUM_SAVED_CRASHDUMP_ENTRIES;
            savedEntries = &savedCrashdumpEntries;
            break;
        default:
            earliestEntryPtr->delete_();
            return;
    }

    log<level::INFO>(
        fmt::format("dump_manager_faultlog.cpp: in saveEarliestEntry(). "
                    "entry id: {}, type: {}, savedEntries->size(): {}",
                    earliestEntryId, static_cast<uint32_t>(earliestEntryType),
                    savedEntries->size())
            .c_str());

    // Check whether saved entries map has space for a new entry
    if (savedEntries->size() < maxNumSavedEntries)
    {
        // Update earliest entry's parent map pointer to its saved entries map
        dynamic_cast<phosphor::dump::faultlog::Entry*>(earliestEntryPtr)
            ->parentMap = savedEntries;

        // Insert earliest entry into saved entries map
        savedEntries->insert(
            std::make_pair(earliestEntryId, std::move(earliestEntry->second)));

        // Erase earliest entry from fault log entries map
        entries.erase(earliestEntryId);
    }
    else
    {
        // Delete earliest entry
        earliestEntryPtr->delete_();
    }
}

} // namespace faultlog
} // namespace dump
} // namespace phosphor
