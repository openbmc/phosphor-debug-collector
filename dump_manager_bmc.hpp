#pragma once

#include "dump_file_helper.hpp"
#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "errors_map.hpp"

#include <sdeventplus/source/child.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

#include <filesystem>
#include <map>

namespace phosphor
{
namespace dump
{
namespace bmc
{

using CreateIface = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Dump::server::Create>;

using ::sdeventplus::source::Child;

/** @class Manager
 *  @brief OpenBMC Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Create DBus API
 */
class Manager :
    virtual public CreateIface,
    virtual public phosphor::dump::Manager
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
    Manager(sdbusplus::bus_t& bus, const EventPtr& event, const char* path,
            const std::string& baseEntryPath, const char* filePath) :
        CreateIface(bus, path),
        phosphor::dump::Manager(bus, path, baseEntryPath),
        eventLoop(event.get()), dumpDir(filePath)
    {
        dumpWatch = std::make_unique<DumpStorageWatch<Manager>>(event, filePath, BMC_DUMP_FILENAME_REGEX, *this);
    }

    /** @brief Construct dump d-bus objects from their persisted
     *        representations.
     */
    void restore() override
    {
        dumpWatch->restore();
    }

    /** @brief Implementation for CreateDump
     *  Method to create a BMC dump entry when user requests for a new BMC dump
     *
     *  @return object_path - The object path of the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(phosphor::dump::DumpCreateParams params) override;

    /** @brief Create a Dump Entry Object
     *  @param[in] id - Id of the dump
     *  @param[in] objPath - Object path to attach to
     *  @param[in] ms - Dump creation timestamp since the epoch.
     *  @param[in] fileSize - Dump file size in bytes.
     *  @param[in] file - Name of dump file.
     *  @param[in] status - status of the dump.
     *  @param[in] originatorId - Originator id of the dump.
     *  @param[in] originatorType - Originator type of the dump.
     */

    void createEntry(const uint32_t id, const uint64_t ms, uint64_t fileSize,
                     const std::filesystem::path& file,
                     phosphor::dump::OperationStatus status,
                     std::string originatorId, OriginatorTypes originatorType);

  private:
    /** @brief Capture BMC Dump based on the Dump type.
     *  @param[in] type - Type of the dump to pass to dreport
     *  @param[in] path - An absolute path to the file
     *             to be included as part of Dump package.
     *  @return id - The Dump entry id number.
     */
    uint32_t captureDump(const std::string& type, const std::string& path);

    /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

    /** @brief Path to the dump file*/
    std::string dumpDir;

    std::unique_ptr<DumpStorageWatch<Manager>> dumpWatch;

    /** @brief Flag to reject user intiated dump if a dump is in progress*/
    // TODO: https://github.com/openbmc/phosphor-debug-collector/issues/19
    static bool fUserDumpInProgress;

    /** @brief map of SDEventPlus child pointer added to event loop */
    std::map<pid_t, std::unique_ptr<Child>> childPtrMap;
};

} // namespace bmc
} // namespace dump
} // namespace phosphor
