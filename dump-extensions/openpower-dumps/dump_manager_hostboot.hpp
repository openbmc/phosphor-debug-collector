#pragma once

#include "dump-extensions/openpower-dumps/opconfig.h"

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
namespace hostboot
{

constexpr auto FILENAME_REGEX = "hbdump_([0-9]+)_([0-9]+).([a-zA-Z0-9]+)";
using CreateIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Dump::server::Create,
    sdbusplus::com::ibm::Dump::server::Create,
    sdbusplus::xyz::openbmc_project::Dump::server::NewDump>;

using UserMap = phosphor::dump::inotify::UserMap;

using Watch = phosphor::dump::inotify::Watch;

/** @class Manager
 *  @brief Hostboot Dump  manager implementation.
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
            bus, event, path, baseEntryPath, filePath, FILENAME_REGEX,
            HOSTBOOT_DUMP_MAX_SIZE, HOSTBOOT_DUMP_MIN_SPACE_REQD,
            HOSTBOOT_DUMP_TOTAL_SIZE)
    {}

    /** @brief Implementation for CreateDump
     *  Method to create a Hostboot dump entry when user requests for a
     *  new Hostboot dump
     *
     *  @return object_path - The object path of the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(std::map<std::string, std::string> params) override;

    /** @brief Notify the Hostboot dump manager about creation of a new dump.
     *  @param[in] dumpId - Id from the source of the dump.
     *  @param[in] size - Size of the dump.
     */
    void notify(uint32_t dumpId, uint64_t size) override;

  private:
    /** @brief Capture a Hostboot Dump based.
     *  @param[in] fullPaths - List of absolute paths to the files
     *             to be included as part of Dump package.
     *  @return id - The Dump entry id number.
     */
    uint32_t captureDump(const std::vector<std::string>& fullPaths,
                         uint32_t dumpId);
};

} // namespace hostboot
} // namespace dump
} // namespace openpower
