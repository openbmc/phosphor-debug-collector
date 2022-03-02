#include "config.h"

#include "dump_manager_faultlog.hpp"

#include "dump_utils.hpp"
#include "faultlog_dump_entry.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <fmt/core.h>
#include <unistd.h>

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
    log<level::WARNING>("In dump_manager_fault.cpp createDump");

    if (params.empty())
    {
        log<level::INFO>("No additional parameters received");
    }
    else
    {
        log<level::INFO>("Got additional paramaters");
    }

    // Get the id
    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = std::filesystem::path(baseEntryPath) / idString;

    log<level::INFO>(fmt::format("objPath={}", objPath.c_str()).c_str());

    // start capture dump code
    log<level::INFO>(
        fmt::format("faultLogFile={}",
                    (std::string(FAULTLOG_DUMP_PATH) + idString).c_str())
            .c_str());

    std::ofstream faultLogFile;

    errno = 0;

    std::filesystem::create_directory(FAULTLOG_DUMP_PATH);
    faultLogFile.open((std::string(FAULTLOG_DUMP_PATH) + idString).c_str(),
                      std::ofstream::out | std::fstream::trunc);

    auto openError = errno;
    log<level::INFO>(fmt::format("open errno is {}", openError).c_str());

    if (faultLogFile.is_open())
    {
        log<level::INFO>("faultLogFile is open");
    }
    else
    {
        log<level::INFO>("faultLogFile is not open");
    }

    std::string entryTypeStr, mainLogRefStr;

    for (auto& [key, val] : params)
    {
        log<level::INFO>(fmt::format("key={}", key).c_str());
        faultLogFile << "key = " << key << ", ";
        std::visit(
            [&faultLogFile, &key, &entryTypeStr, &mainLogRefStr](auto value) {
                std::ostringstream oss;
                oss << value;
                log<level::INFO>(fmt::format("value={}", oss.str()).c_str());
                faultLogFile << "value = " << oss.str() << ", ";

                if constexpr (std::is_same_v<std::decay_t<decltype(value)>,
                                             std::string>)
                {
                    if (key.compare("EntryType") == 0)
                    {
                        entryTypeStr = value;
                    }
                    else if (key.compare("MainLogRef") == 0)
                    {
                        mainLogRefStr = value;
                    }
                }
            },
            val);
    }

    // faultLogFile << "This is faultlog file #" << idString << " at "
    //             << std::string(FAULTLOG_DUMP_PATH) + idString << "\n";

    faultLogFile.close();

    try
    {
        uint64_t timeStamp =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();

        log<level::INFO>(
            fmt::format("dump_manager_faultlog.cpp, timeStamp = {}", timeStamp)
                .c_str());

        log<level::INFO>(
            fmt::format(
                "dump_manager_faultlog.cpp: currFaultLogIdx = {}, id = {}",
                currFaultLogIdx, id)
                .c_str());

        if (id > MAX_FAULT_LOG_ENTRIES)
        {
            // Check if need to move to saved entries
            // TODO: saved entries to expire after a time
            if (numSavedEntries < MAX_SAVED_ENTRIES)
            {
                log<level::INFO>(fmt::format("dump_manager_faultlog.cpp: save "
                                             "entry: numSavedEntries = {}",
                                             numSavedEntries)
                                     .c_str());

                faultLogEntries.push_back(
                    std::move(faultLogEntries.at(currFaultLogIdx)));

                numSavedEntries++;
            }
            else
            {
                uint32_t prevId = (faultLogEntries.at(currFaultLogIdx))
                                      ->getId(); // TODO: check for NULL pointer

                log<level::INFO>("dump_manager_faultlog.cpp: delete_ entry");

                // delete old entry from dbus
                (faultLogEntries.at(currFaultLogIdx))->delete_();

                log<level::INFO>(
                    fmt::format("dump_manager_faultlog.cpp: remove {}",
                                std::string(FAULTLOG_DUMP_PATH) +
                                    std::to_string(prevId))
                        .c_str());

                std::filesystem::remove(std::string(FAULTLOG_DUMP_PATH) +
                                        std::to_string(prevId));
            }
        }

        log<level::INFO>("dump_manager_faultlog.cpp: add to faultLogEntries");

        faultLogEntries[currFaultLogIdx] = std::make_unique<faultlog::Entry>(
            bus, objPath.c_str(), id, timeStamp, 0, std::string(),
            phosphor::dump::OperationStatus::Completed, entryTypeStr,
            mainLogRefStr, *this);

        log<level::INFO>(
            "dump_manager_faultlog.cpp: after add to faultLogEntries");
        log<level::INFO>(
            fmt::format("dump_manager_faultlog.cpp newEntryId: {}",
                        (faultLogEntries.at(currFaultLogIdx))->getId())
                .c_str());
    }
    catch (const std::invalid_argument& e)
    {
        log<level::ERR>(fmt::format("Error in creating dump entry, "
                                    "errormsg({}), OBJECTPATH({}), ID({})",
                                    e.what(), objPath.c_str(), id)
                            .c_str());
        elog<InternalFailure>();
    }
    catch (...)
    {
        log<level::ERR>("dump_manager_faultlog.cpp: Caught other error");
    }

    lastEntryId++;
    currFaultLogIdx = (currFaultLogIdx + 1) % MAX_FAULT_LOG_ENTRIES;

    log<level::WARNING>("End of dump_manager_faultlog.cpp createDump");
    return objPath.string();
}

void Manager::init()
{
    crashdumpMatch = std::make_unique<sdbusplus::bus::match::match>(
        bus,
        "type='signal',interface='com.intel.crashdump.Stored',member='"
        "CrashdumpComplete'",
        [this](sdbusplus::message::message& msg) {
            if (msg.is_method_error())
            {
                log<level::INFO>(
                    "dump_manager_faultlog got crashdump complete error!");
            }

            log<level::INFO>("dump_manager_faultlog got crashdump complete!");

            const std::filesystem::path crashdumpDir = "/tmp/crashdump/output/";

            std::string newestCrashdumpFileName("none");
            uint64_t newestCrashdumpTime = 0;

            // Temporary hack to make sure the crashdump is done being written
            // before we search for the most recent crashdump written
            sleep(3);

            // Search for the most recent crashdump written
            for (const auto& i :
                 std::filesystem::directory_iterator(crashdumpDir))
            {

                std::string currFileName = i.path().filename().string();
                if (i.is_regular_file() &&
                    (currFileName.compare(0, sizeof("crashdump_") - 1,
                                          "crashdump_") == 0))
                {
                    log<level::INFO>(
                        fmt::format("crashdumpfile: {}", currFileName).c_str());

                    uint64_t lastWriteTime =
                        std::chrono::system_clock::to_time_t(
                            std::chrono::file_clock::to_sys(
                                std::filesystem::last_write_time(
                                    crashdumpDir / currFileName)));

                    log<level::INFO>(
                        fmt::format("lastWriteTime: {}", lastWriteTime)
                            .c_str());

                    if (lastWriteTime > newestCrashdumpTime)
                    {
                        newestCrashdumpTime = lastWriteTime;
                        newestCrashdumpFileName = currFileName;
                    }
                }
            }

            std::map<std::string, std::variant<std::string, uint64_t>>
                crashdumpMap;

            crashdumpMap.insert(
                std::pair<std::string, std::string>("EntryType", "ACD"));

            crashdumpMap.insert(std::pair<std::string, std::string>(
                "MainLogRef", newestCrashdumpFileName));

            createDump(crashdumpMap);
        });
}

} // namespace faultlog
} // namespace dump
} // namespace phosphor
