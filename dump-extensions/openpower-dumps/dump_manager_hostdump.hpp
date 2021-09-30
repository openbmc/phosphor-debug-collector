#pragma once

#include "dump-extensions/openpower-dumps/openpower_dumps_config.h"

#include "bmcstored_dump_entry.hpp"
#include "dump_helper.hpp"
#include "dump_utils.hpp"
#include "op_dump_util.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"

#include <fmt/core.h>
#include <sys/inotify.h>
#include <systemd/sd-event.h>
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
    sdbusplus::com::ibm::Dump::server::Create>;

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
class Manager : virtual public CreateIface, public phosphor::dump::Manager
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
     *  @param[in] maxDumpSize - Maximum allowed size of dump file
     *  @param[in] minDumpSize - Minimum size of a usable dump
     *  @param[in] allocatedSize - Total allocated space for the dump.
     */
    Manager(sdbusplus::bus::bus& bus, const phosphor::dump::EventPtr& event,
            const char* path, const std::string& baseEntryPath,
            uint32_t startingId, const char* filePath,
            const std::string dumpNamePrefix, const uint64_t maxDumpSize,
            const uint64_t minDumpSize, const uint64_t allocatedSize,
            const uint8_t dumpType) :
        CreateIface(bus, path),
        phosphor::dump::Manager(bus, path, baseEntryPath),
        dumpNamePrefix(dumpNamePrefix), dumpType(dumpType),
        dumpHelper(bus, event, filePath,
                   dumpNamePrefix + HOST_DUMP_COMMON_FILENAME_PART, maxDumpSize,
                   minDumpSize, allocatedSize, this)
    {
        updateLastEntryId(startingId);
    }

    /** @brief Implementation for CreateDump
     *  Method to create a host dump entry when user requests for a
     *  new host dump
     *
     *  @return object_path - The object path of the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(phosphor::dump::DumpCreateParams params) override
    {
        // Check dump policy
        util::isOPDumpsEnabled();
        uint8_t dumpType = 0;
        uint64_t eid = 0;
        uint64_t failingUnit = 0;

        openpower::dump::util::extractDumpCreateParams(params, dumpType, eid,
                                                       failingUnit);

        uint32_t id = captureDump(eid, failingUnit);

        // Entry Object path.
        auto objPath = std::filesystem::path(baseEntryPath) /
                       std::to_string(id);

        lg2::info("Create dump type: {DUMP_TYPE} with id: {ID} ", "DUMP_TYPE",
                  dumpNamePrefix, "ID", id);

        std::time_t timeStamp = std::time(nullptr);
        createEntry(id, objPath, timeStamp, 0, std::string(),
                    phosphor::dump::OperationStatus::InProgress, std::string(),
                    originatorTypes::Internal);

        return objPath.string();
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
                id, std::make_unique<phosphor::dump::bmc_stored::Entry<T>>(
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

    /** @brief Construct dump d-bus objects from their persisted
     *        representations.
     */
    void restore() override
    {
        dumpHelper.restore();
    }

  private:
    std::string dumpNamePrefix;
    uint8_t dumpType;

    /** @brief Capture the dump
     *  @param[in] eid - Error log id associated with the dump
     *  @param[in] failingUnit - Hardware unit failed
     *
     *  @return The id of the dump getting created
     */
    int captureDump(uint64_t eid, uint64_t failingUnit)
    {
        std::string idStr;

        auto dumpId = lastEntryId + 1;
        try
        {
            idStr = std::to_string(dumpId);
        }
        catch (std::exception& e)
        {
            lg2::error("Dump capture: Error converting idto string");
            throw std::runtime_error(
                "Dump capture: Error converting dump id to string");
        }

        // Get Dump size.
        // TODO #ibm-openbmc/issues/3061
        // Dump request will be rejected if there is not enough space for
        // one complete dump, change this behavior to crate a partial dump
        // with available space.
        auto size = dumpHelper.getAllowedSize();

        pid_t pid = fork();
        if (pid == 0)
        {
            std::filesystem::path dumpPath(dumpHelper.dumpDir);
            dumpPath /= idStr;
            std::stringstream stream;
            stream << std::setfill('0') << std::setw(8) << std::hex << eid;
            std::string eidStr(stream.str());

            lg2::info(
                "Creating dump with dumpPath: {DUMP_PATH} id:{ID} available "
                "space: {SPACE_AVAILABLE} eid: {EID} dump type: {DUMP_TYPE} failing unit: {FAILING_UNIT}",
                "DUMP_PATH", dumpPath.string(), "ID", dumpId, "SPACE_AVAILABLE",
                size, "EID", eidStr, "DUMP_TYPE", dumpType, "FAILING_UNIT",
                failingUnit);
            execl("/usr/bin/opdreport", "opdreport", "-d", dumpPath.c_str(),
                  "-i", idStr.c_str(), "-s", std::to_string(size).c_str(), "-e",
                  eidStr.c_str(), "-t", std::to_string(dumpType).c_str(), "-f",
                  std::to_string(failingUnit).c_str(), nullptr);

            // opdreport script execution is failed.
            auto error = errno;
            lg2::error(
                "Dump capture: Error occurred during "
                "opdreport function execution, errno: {ERRONO}, dumpPrefix: {DUMP_NAME}, "
                "dumpPath: {DUMP_PATH}, allowedSize: {ALLOWED_SIZE}",
                "ERRONO", error, "DUMP_NAME", dumpNamePrefix.c_str(),
                "DUMP_PATH", dumpPath.c_str(), "ALLOWED_SIZE", size);
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
            Child::Callback callback =
                [this, dumpEntry, pid](Child&, const siginfo_t* si) {
                // Set progress as failed if packaging return error
                if (si->si_status != 0)
                {
                    lg2::error("Dump packaging failed");
                    if (dumpEntry != nullptr)
                    {
                        reinterpret_cast<phosphor::dump::Entry*>(dumpEntry)
                            ->status(phosphor::dump::OperationStatus::Failed);
                    }
                }
                else
                {
                    lg2::info("Dump packaging completed");
                }
                this->childPtrMap.erase(pid);
            };
            try
            {
                childPtrMap.emplace(
                    pid, std::make_unique<Child>(dumpHelper.eventLoop.get(),
                                                 pid, WEXITED | WSTOPPED,
                                                 std::move(callback)));
            }
            catch (const sdeventplus::SdEventError& ex)
            {
                // Failed to add to event loop
                lg2::error("Dump capture: Error occurred during "
                           "the sdeventplus::source::Child error: {ERROR_MSG}",
                           "ERROR_MSG", ex.what());
                throw std::runtime_error(
                    "Dump capture: Error occurred during the "
                    "sdeventplus::source::Child creation");
            }
        }
        else
        {
            auto error = errno;
            lg2::error(
                "Dump capture: Error occurred during fork, errno: {ERRORNO}",
                "ERRORNO", error);
            throw std::runtime_error(
                "Dump capture: Error occurred during fork");
        }
        return ++lastEntryId;
    }

    /** @brief map of SDEventPlus child pointer added to event loop */
    std::map<pid_t, std::unique_ptr<Child>> childPtrMap;

    phosphor::dump::util::DumpHelper dumpHelper;
};

} // namespace hostdump
} // namespace dump
} // namespace openpower
