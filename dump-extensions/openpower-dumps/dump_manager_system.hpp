#pragma once

#include "base_dump_manager.hpp"
#include "com/ibm/Dump/Notify/server.hpp"
#include "dump_helper.hpp"
#include "dump_types.hpp"
#include "dump_utils.hpp"
#include "host_transport_exts.hpp"
#include "new_dump_entry.hpp"
#include "op_dump_util.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

#include <map>
#include <unordered_map>

namespace openpower
{
namespace dump
{

constexpr uint32_t INVALID_SOURCE_ID = 0xFFFFFFFF;
using NotifyIface =
    sdbusplus::server::object_t<sdbusplus::com::ibm::Dump::server::Notify>;
using NotifyDumpType = sdbusplus::common::com::ibm::dump::Notify::DumpType;

template <typename T>
concept DumpEntryClass = std::derived_from<T, phosphor::dump::BaseEntry>;

/** @class Manager
 *  @brief System Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Notify DBus API
 */
class Manager :
    virtual public NotifyIface,
    virtual public phosphor::dump::BaseManager
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
     *  @param[in] baseEntryPath - Base path of the dump entry.
     */
    Manager(sdbusplus::bus_t& bus, const char* path,
            const std::string& baseEntryPath,
            phosphor::dump::host::HostTransport& hostTransport);

    void restore() override
    {
        // TODO #2597  Implement the restore to restore the dump entries
        // after the service restart.
    }

    /** @brief Notify the system dump manager about creation of a new dump.
     *  @param[in] dumpId - Id from the source of the dump.
     *  @param[in] size - Size of the dump.
     */
    void notifyDump(uint32_t dumpId, uint64_t size, NotifyDumpType type,
                    uint32_t token = 0) override;

    /** @brief Implementation for CreateDump
     *  Method to create a new system dump entry when user
     *  requests for a new system dump.
     *
     *  @return object_path - The path to the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(phosphor::dump::DumpCreateParams params) override;

    inline uint32_t getNextId(phosphor::dump::DumpTypes type)
    {
        try
        {
            // Return the LastEntryId value associated with the given dump type.
            return std::get<0>(dumpEntries.at(type)) + 1;
        }
        catch (const std::out_of_range&)
        {
            return 1;
        }
    }

    inline std::string& getBaseEntryPath()
    {
        return baseEntryPath;
    }

    template <DumpEntryClass T>
    inline void insertToDumpEntries(phosphor::dump::DumpTypes type,
                                    std::unique_ptr<T> entry)
    {
        auto& [lastEntryId, entries] = dumpEntries[type];

        // Determine the new last entry ID.
        std::get<0>(dumpEntries[type]) = std::max(lastEntryId + 1, entry->getDumpId());

        auto baseEntry =
            std::unique_ptr<phosphor::dump::BaseEntry>(entry.release());
        

        // Insert the entry.
        entries.emplace(baseEntry->getDumpId(), std::move(baseEntry));
    }

  private:
    phosphor::dump::host::HostTransport& hostTransport;

    phosphor::dump::BaseEntry*
        getInProgressEntry(phosphor::dump::DumpTypes type, uint32_t dumpId,
                           uint64_t size, uint32_t token = 0);

    void erase(uint32_t /*entryId*/) override {}
    void deleteAll() {}

    using DumpEntryList =
        std::map<uint32_t, std::unique_ptr<phosphor::dump::BaseEntry>>;
    using LastEntryId = uint32_t;
    std::unordered_map<phosphor::dump::DumpTypes,
                       std::tuple<LastEntryId, DumpEntryList>>
        dumpEntries;

    std::map<phosphor::dump::DumpTypes, std::unique_ptr<DumpHelper>> helpers;

    /** @brief base object path for the entry object */
    std::string baseEntryPath;
};

} // namespace dump
} // namespace openpower
