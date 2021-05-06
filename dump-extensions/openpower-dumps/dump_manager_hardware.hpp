#pragma once

#include "dump-extensions/openpower-dumps/openpower_dumps_config.h"

#include "dump_manager_bmcstored.hpp"
#include "dump_utils.hpp"
#include "watch.hpp"
#include "xyz/openbmc_project/Dump/NewDump/server.hpp"

#include <com/ibm/Dump/Create/server.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

#include <filesystem>

namespace openpower
{
namespace dump
{
namespace hardware
{

constexpr auto HARDWARE_DUMP_FILENAME_REGEX = "hwdump_([0-9]+)_([0-9]+).([a-zA-Z0-9]+)";
using CreateIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Dump::server::Create,
    sdbusplus::com::ibm::Dump::server::Create,
    sdbusplus::xyz::openbmc_project::Dump::server::NewDump>;

/** @class Manager
 *  @brief Hardware Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Create DBus API
 */
class Manager :
    virtual public CreateIface,
    virtual public phosphor::dump::bmc_stored::Manager
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
            const char* filePath) :
        CreateIface(bus, path),
        phosphor::dump::bmc_stored::Manager(
            bus, event, path, baseEntryPath, filePath, HARDWARE_DUMP_FILENAME_REGEX,
            HARDWARE_DUMP_MAX_SIZE, HARDWARE_DUMP_MIN_SPACE_REQD,
            HARDWARE_DUMP_TOTAL_SIZE)
    {}

    /** @brief Implementation for CreateDump
     *  Method to create a Hardware dump entry when user requests for a
     *  new Hardware dump
     *
     *  @return object_path - The object path of the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(phosphor::dump::DumpCreateParams params) override;

    /** @brief Notify the Hardware dump manager about creation of a new dump.
     *  @param[in] dumpId - Id from the source of the dump.
     *  @param[in] size - Size of the dump.
     */
    void notify(uint32_t dumpId, uint64_t size) override;

    /** @brief Create a  Dump Entry Object
     *  @param[in] id - Id of the dump
     *  @param[in] objPath - Object path to attach to
     *  @param[in] ms - Dump creation timestamp
     *             since the epoch.
     *  @param[in] fileSize - Dump file size in bytes.
     *  @param[in] file - Name of dump file.
     *  @param[in] status - status of the dump.
     */
    void createEntry(const uint32_t id, const std::string objPath,
                     const uint64_t ms, uint64_t fileSize,
                     const std::filesystem::path& file,
                     phosphor::dump::OperationStatus status) override;
};

} // namespace hardware
} // namespace dump
} // namespace openpower
