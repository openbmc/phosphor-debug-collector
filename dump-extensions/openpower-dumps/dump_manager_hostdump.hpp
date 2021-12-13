#pragma once

#include "dump-extensions/openpower-dumps/openpower_dumps_config.h"

#include "dump_manager_bmcstored.hpp"
#include "dump_utils.hpp"
#include "host_dump_entry.hpp"
#include "op_dump_util.hpp"
#include "watch.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"
#include "xyz/openbmc_project/Dump/NewDump/server.hpp"

#include <fmt/core.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <com/ibm/Dump/Create/server.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <sdeventplus/exception.hpp>
#include <sdeventplus/source/base.hpp>
#include <sdeventplus/source/child.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

#include <ctime>
#include <filesystem>
#include <regex>

namespace openpower
{
namespace dump
{
namespace hostdump
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

constexpr auto INVALID_DUMP_SIZE = 0;
constexpr auto HOST_DUMP_COMMON_FILENAME_PART =
    "_([0-9]+)_([0-9]+).([a-zA-Z0-9]+)";

using CreateIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Dump::server::Create,
    sdbusplus::com::ibm::Dump::server::Create,
    sdbusplus::xyz::openbmc_project::Dump::server::NewDump>;

using UserMap = phosphor::dump::inotify::UserMap;

using Watch = phosphor::dump::inotify::Watch;
using ::sdeventplus::source::Child;

using originatorTypes = sdbusplus::xyz::openbmc_project::Common::server::
    OriginatedBy::OriginatorTypes;

/** @class Manager
 *  @brief Host Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Create
 *  com::ibm::Dump::Create and
 *  xyz::openbmc_project::Dump::NewDump D-Bus APIs
 */
template <typename T>
class Manager :
    virtual public CreateIface,
    public phosphor::dump::bmc_stored::Manager
{
  public:
    Manager() = delete;
    Manager(const Manager&) = default;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    virtual ~Manager() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] event - Dump manager sd_event loop.
     *  @param[in] path - Path to attach at.
     *  @param[in] baseEntryPath - Base path for dump entry.
     *  @param[in] startingId - Starting dump id
     *  @param[in] filePath - Path where the dumps are stored.
     *  @param[in] dumpNamePrefix - Prefix to the dump filename
     *  @param[in] dumpTempFileDir - Temporary location of dump files
     *  @param[in] maxDumpSize - Maximum allowed size of dump file
     *  @param[in] minDumpSize - Minimum size of a usable dump
     *  @param[in] allocatedSize - Total allocated space for the dump.
     */
    Manager(sdbusplus::bus::bus& bus, const phosphor::dump::EventPtr& event,
            const char* path, const std::string& baseEntryPath,
            uint32_t startingId, const char* filePath,
            const std::string dumpNamePrefix, const std::string dumpTempFileDir,
            const uint64_t maxDumpSize, const uint64_t minDumpSize,
            const uint64_t allocatedSize) :
        CreateIface(bus, path),
        phosphor::dump::bmc_stored::Manager(
            bus, event, path, baseEntryPath, startingId, filePath,
            dumpNamePrefix + HOST_DUMP_COMMON_FILENAME_PART, maxDumpSize,
            minDumpSize, allocatedSize),
        dumpNamePrefix(dumpNamePrefix), dumpTempFileDir(dumpTempFileDir)
    {}

    /** @brief Implementation for CreateDump
     *  Method to create a host dump entry when user requests for a
     *  new host dump
     *
     *  @return object_path - The object path of the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(phosphor::dump::DumpCreateParams params) override
    {
        using InvalidArgument =
            sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
        using Argument = xyz::openbmc_project::Common::InvalidArgument;
        if (!params.empty())
        {
            log<level::ERR>(fmt::format("Dump type({}) accepts no additional "
                                        "parameters, number of parameters({})",
                                        dumpNamePrefix, params.size())
                                .c_str());
            elog<InvalidArgument>(
                Argument::ARGUMENT_NAME("NO_PARAMETERS_NEEDED"),
                Argument::ARGUMENT_VALUE("INVALID_PARAMETERS"));
        }

        // Check dump policy
        util::isOPDumpsEnabled();

        auto size = getAllowedSize();

        uint32_t id = ++lastEntryId;

        // Entry Object path.
        auto objPath =
            std::filesystem::path(baseEntryPath) / std::to_string(id);

        log<level::INFO>(fmt::format("Create dump type({}) with id({}) "
                                     "available space: ({}) kilobytes",
                                     dumpNamePrefix, id, size)
                             .c_str());

        std::time_t timeStamp = std::time(nullptr);
        createEntry(id, objPath, timeStamp, 0, std::string(),
                    phosphor::dump::OperationStatus::InProgress, std::string(),
                    originatorTypes::Internal);

        return objPath.string();
    }

    /** @brief Notify the host dump manager about creation of a new dump.
     *  @param[in] dumpId - Id from the source of the dump.
     *  @param[in] size - Size of the dump.
     */
    void notify(uint32_t dumpId, uint64_t) override
    {
        try
        {
            captureDump(dumpId);
        }
        catch (std::exception& e)
        {
            log<level::ERR>(
                fmt::format("Failed to package dump({}): id({}) errorMsg({})",
                            dumpNamePrefix, dumpId, e.what())
                    .c_str());
            throw std::runtime_error("Failed to package dump");
        }
    }
    /** @brief Create a  Dump Entry Object
     *  @param[in] id - Id of the dump
     *  @param[in] objPath - Object path to attach to
     *  @param[in] ms - Dump creation timestamp since the epoch.
     *  @param[in] fileSize - Dump file size in bytes.
     *  @param[in] file - Name of dump file.
     *  @param[in] status - status of the dump.
     *  @param[in] originatorId - Id of the originator of the dump
     *  @param[in] originatorType - Originator type
     */

    virtual void createEntry(const uint32_t id, const std::string objPath,
                             const uint64_t ms, uint64_t fileSize,
                             const std::filesystem::path& file,
                             phosphor::dump::OperationStatus status,
                             std::string originatorId,
                             originatorTypes originatorType) override
    {
        try
        {
            entries.insert(std::make_pair(
                id, std::make_unique<openpower::dump::hostdump::Entry<T>>(
                        bus, objPath.c_str(), id, ms, fileSize, file, status,
                        originatorId, originatorType, *this)));
        }
        catch (const std::invalid_argument& e)
        {
            log<level::ERR>(fmt::format("Error in creating host dump entry, "
                                        "errormsg({}), OBJECTPATH({}), ID({})",
                                        e.what(), objPath.c_str(), id)
                                .c_str());
            throw std::runtime_error("Error in creating host dump entry");
        }
    }

  private:
    std::string dumpNamePrefix;
    std::string dumpTempFileDir;

    void captureDump(uint32_t dumpId)
    {
        std::string idStr;
        try
        {
            idStr = std::to_string(dumpId);
        }
        catch (std::exception& e)
        {
            log<level::ERR>("Dump capture: Error converting idto string");
            throw std::runtime_error(
                "Dump capture: Error converting dump id to string");
        }

        // Get Dump size.
        // TODO #ibm-openbmc/issues/3061
        // Dump request will be rejected if there is not enough space for
        // one complete dump, change this behavior to crate a partial dump
        // with available space.
        auto size = getAllowedSize();

        auto dumpTempPath = std::filesystem::path(dumpTempFileDir) / idStr;

        pid_t pid = fork();
        if (pid == 0)
        {
            std::filesystem::path dumpPath(dumpDir);
            dumpPath /= idStr;
            execl("/usr/bin/opdreport", "opdreport", "-d", dumpPath.c_str(),
                  "-i", idStr.c_str(), "-s", std::to_string(size).c_str(), "-q",
                  "-v", "-p", dumpTempPath.c_str(), "-n",
                  dumpNamePrefix.c_str(), nullptr);

            // opdreport script execution is failed.
            auto error = errno;
            log<level::ERR>(
                fmt::format(
                    "Dump capture: Error occurred during "
                    "opdreport function execution, errno({}), dumpPrefix({}), "
                    "dumpPath({}), dumpSourcePath({}), allowedSize({})",
                    error, dumpNamePrefix.c_str(), dumpPath.c_str(),
                    dumpTempPath.c_str(), size)
                    .c_str());
            throw std::runtime_error("Dump capture: Error occured during "
                                     "opdreport script execution");
        }
        else if (pid > 0)
        {
            phosphor::dump::Entry* dumpEntry = NULL;
            auto dumpIt = entries.find(dumpId);
            if (dumpIt != entries.end())
            {
                dumpEntry = dumpIt->second.get();
            }
            Child::Callback callback = [this, dumpEntry,
                                        pid](Child&, const siginfo_t* si) {
                // Set progress as failed if packaging return error
                if (si->si_status != 0)
                {
                    log<level::ERR>("Dump packaging failed");
                    if (dumpEntry != nullptr)
                    {
                        reinterpret_cast<phosphor::dump::Entry*>(dumpEntry)
                            ->status(phosphor::dump::OperationStatus::Failed);
                    }
                }
                else
                {
                    log<level::INFO>("Dump packaging completed");
                }
                this->childPtrMap.erase(pid);
            };
            try
            {
                childPtrMap.emplace(
                    pid, std::make_unique<Child>(eventLoop.get(), pid,
                                                 WEXITED | WSTOPPED,
                                                 std::move(callback)));
            }
            catch (const sdeventplus::SdEventError& ex)
            {
                // Failed to add to event loop
                log<level::ERR>(
                    fmt::format("Dump capture: Error occurred during "
                                "the sdeventplus::source::Child ex({})",
                                ex.what())
                        .c_str());
                throw std::runtime_error(
                    "Dump capture: Error occurred during the "
                    "sdeventplus::source::Child creation");
            }
        }
        else
        {
            auto error = errno;
            log<level::ERR>(
                fmt::format(
                    "Dump capture: Error occurred during fork, errno({})",
                    error)
                    .c_str());
            throw std::runtime_error(
                "Dump capture: Error occurred during fork");
        }
    }
};

} // namespace hostdump
} // namespace dump
} // namespace openpower
