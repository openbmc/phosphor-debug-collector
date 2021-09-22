#pragma once

#include "dump_entry.hpp"
#include "xyz/openbmc_project/Dump/Entry/System/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

namespace openpower
{
namespace dump
{
namespace system
{
template <typename T>
using ServerObject = typename sdbusplus::server::object::object<T>;

using EntryIfaces = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Dump::Entry::server::System>;

class Manager;

/** @class Entry
 *  @brief System Dump Entry implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Entry DBus API
 */
class Entry : virtual public EntryIfaces, virtual public phosphor::dump::Entry
{
  public:
    Entry() = delete;
    Entry(const Entry&) = delete;
    Entry& operator=(const Entry&) = delete;
    Entry(Entry&&) = delete;
    Entry& operator=(Entry&&) = delete;
    ~Entry() = default;

    /** @brief Constructor for the Dump Entry Object
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Object path to attach to
     *  @param[in] dumpId - Dump id.
     *  @param[in] timeStamp - Dump creation timestamp
     *             since the epoch.
     *  @param[in] dumpSize - Dump size in bytes.
     *  @param[in] sourceId - DumpId provided by the source.
     *  @param[in] status - status  of the dump.
     *  @param[in] baseEntryPath - Base entry path
     *  @param[in] parent - The dump entry's parent.
     *  @param[in] emitSignal - Default true, this to emit the signal for dump
     *             object
     */
    Entry(sdbusplus::bus::bus& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t dumpSize, const uint32_t sourceId,
          phosphor::dump::OperationStatus status,
          const std::string& baseEntryPath, phosphor::dump::Manager& parent,
          bool emitSignal = true) :
        EntryIfaces(bus, objPath.c_str(), true),
        phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, dumpSize,
                              status, parent),
        baseEntryPath(baseEntryPath)
    {
        sourceDumpId(sourceId);
        // Emit deferred signal.
        if (emitSignal)
            this->openpower::dump::system::EntryIfaces::emit_object_added();
    };

    /** @brief Method to initiate the offload of dump
     *  @param[in] uri - URI to offload dump.
     */
    void initiateOffload(std::string uri) override;

    /** @brief Method to update an existing dump entry
     *  @param[in] timeStamp - Dump creation timestamp
     *  @param[in] dumpSize - Dump size in bytes.
     *  @param[in] sourceId - DumpId provided by the source.
     */
    void update(uint64_t timeStamp, uint64_t dumpSize, const uint32_t sourceId);

    /**
     * @brief Delete host system dump and it entry dbus object
     */
    void delete_() override;

    /** @brief Function to get the dump ID
     *
     *  @return Dump ID
     */
    uint32_t getID() const
    {
        return id;
    }

    /** @brief Function to set the dump ID
     *
     *  @return DumpId
     */
    void setID(uint32_t dumpId)
    {
        id = dumpId;
    }

  private:
    /** @brief Based entry path of dumps*/
    std::string baseEntryPath;
};

} // namespace system
} // namespace dump
} // namespace openpower
