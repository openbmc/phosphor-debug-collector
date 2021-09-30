#pragma once

#include "dump-extensions/openpower-dumps/openpower_dumps_config.h"

#include "dump_manager_bmcstored.hpp"
#include "dump_utils.hpp"
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

using CreateIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Dump::server::Create,
    sdbusplus::com::ibm::Dump::server::Create,
    sdbusplus::xyz::openbmc_project::Dump::server::NewDump>;

using UserMap = phosphor::dump::inotify::UserMap;

using Watch = phosphor::dump::inotify::Watch;

/** @class Manager
 *  @brief Hostboot Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Create
 *  com::ibm::Dump::Create and
 *  xyz::openbmc_project::Dump::NewDump D-Bus APIs
 */
template <class T>
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
     *  @param[in] filePath - Path where the dumps are stored.
     */
    Manager(sdbusplus::bus::bus& bus, const phosphor::dump::EventPtr& event,
            const char* path, const std::string& baseEntryPath,
            const char* filePath, const std::string dumpFilenameFormat,
            const std::string dumpNameSuffix, const std::string dumpTempFileDir,
            const uint64_t maxDumpSize, const uint64_t minDumpSize,
            const uint64_t allocatedSize) :
        CreateIface(bus, path),
        phosphor::dump::bmc_stored::Manager(
            bus, event, path, baseEntryPath, filePath, dumpFilenameFormat,
            maxDumpSize, minDumpSize, allocatedSize),
        dumpNameSuffix(dumpNameSuffix), dumpTempFileDir(dumpTempFileDir)
    {}

    /** @brief Implementation for CreateDump
     *  Method to create a Hostboot dump entry when user requests for a
     *  new Hostboot dump
     *
     *  @return object_path - The object path of the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(phosphor::dump::DumpCreateParams params) override
    {
        if (!params.empty())
        {
            log<level::ERR>(fmt::format("Dump accepts no additional "
                                        "parameters, number of parameters({})",
                                        params.size())
                                .c_str());
            throw std::runtime_error(
                "Hostboot dump accepts no additional parameters");
        }

        // Check dump policy
        util::isOPDumpsEnabled();

        uint32_t id = ++lastEntryId;
        // Entry Object path.
        auto objPath =
            std::filesystem::path(baseEntryPath) / std::to_string(id);

        std::time_t timeStamp = std::time(nullptr);
        createEntry(id, objPath, timeStamp, 0, std::string(),
                    phosphor::dump::OperationStatus::InProgress);

        return objPath.string();
    }

    /** @brief Notify the Hostboot dump manager about creation of a new dump.
     *  @param[in] dumpId - Id from the source of the dump.
     *  @param[in] size - Size of the dump.
     */
    void notify(uint32_t dumpId, uint64_t) override
    {
        // Get Dump size.
        // TODO #ibm-openbmc/issues/3061
        // Dump request will be rejected if there is not enough space for
        // one complete dump, change this behavior to crate a partial dump
        // with available space.
        auto size = getAllowedSize();
        try
        {
            util::captureDump(dumpId, size, dumpTempFileDir, dumpDir,
                              dumpNameSuffix, eventLoop);
        }
        catch (std::exception& e)
        {
            log<level::ERR>(
                fmt::format("Failed to package dump({}): id({}) errorMsg({})",
                            dumpNameSuffix, dumpId, e.what())
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
     */
    virtual void createEntry(const uint32_t id, const std::string objPath,
                             const uint64_t ms, uint64_t fileSize,
                             const std::filesystem::path& file,
                             phosphor::dump::OperationStatus status) override
    {
        try
        {
            entries.insert(std::make_pair(
                id, std::make_unique<T>(bus, objPath.c_str(), id, ms, fileSize,
                                        file, status, *this)));
        }
        catch (const std::invalid_argument& e)
        {
            log<level::ERR>(
                fmt::format("Error in creating hostboot dump entry, "
                            "errormsg({}), OBJECTPATH({}), ID({})",
                            e.what(), objPath.c_str(), id)
                    .c_str());
            throw std::runtime_error("Error in creating hostboot dump entry");
        }
    }

  private:
    std::string dumpNameSuffix;
    std::string dumpTempFileDir;
};

} // namespace hostdump
} // namespace dump
} // namespace openpower
