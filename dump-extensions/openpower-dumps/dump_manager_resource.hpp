#pragma once

#include "dump_manager.hpp"
#include "xyz/openbmc_project/Dump/NewDump/server.hpp"

#include <com/ibm/Dump/Create/server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

namespace openpower
{
namespace dump
{
namespace resource
{

constexpr uint32_t INVALID_SOURCE_ID = 0xFFFFFFFF;
using NotifyIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Dump::server::Create,
    sdbusplus::com::ibm::Dump::server::Create,
    sdbusplus::xyz::openbmc_project::Dump::server::NewDump>;

/** @class Manager
 *  @brief Resource Dump manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Notify and
 *  xyz.openbmc_project.Dump.Create  DBus APIs
 */
class Manager :
    virtual public NotifyIface,
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
     *  @param[in] path - Path to attach at.
     *  @param[in] baseEntryPath - Base path of the dump entry.
     */
    Manager(sdbusplus::bus::bus& bus, const char* path,
            const std::string& baseEntryPath) :
        NotifyIface(bus, path),
        phosphor::dump::Manager(bus, path, baseEntryPath)
    {}

    void restore() override
    {
        // TODO #2597  Implement the restore to restore the dump entries
        // after the service restart.
    }

    /** @brief Notify the resource dump manager about creation of a new dump.
     *  @param[in] dumpId - Id from the source of the dump.
     *  @param[in] size - Size of the dump.
     */
    void notify(uint32_t dumpId, uint64_t size) override
    {}

    /** @brief Notify the resource dump manager about creation of a new dump.
     *  @param[in] dumpId - Id from the source of the dump.
     *  @param[in] size - Size of the dump.
     *  @param[in] token - A token to find the dump to update
     */
    void notifyWithToken(uint32_t dumpId, uint64_t size,
                         uint32_t token) override;

    /** @brief Implementation for CreateDump
     *  Method to create Dump.
     *
     *  @return object_path - The object path of the new entry.
     */
    sdbusplus::message::object_path
        createDump(phosphor::dump::DumpCreateParams params) override;
};

} // namespace resource
} // namespace dump
} // namespace openpower
