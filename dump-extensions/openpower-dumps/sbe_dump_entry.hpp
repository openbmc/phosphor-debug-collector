#pragma once

#include "bmcstored_dump_entry.hpp"
#include "com/ibm/Dump/Entry/SBE/server.hpp"
#include "xyz/openbmc_project/Dump/Entry/server.hpp"
#include "xyz/openbmc_project/Object/Delete/server.hpp"
#include "xyz/openbmc_project/Time/EpochTime/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

#include <filesystem>

namespace openpower
{
namespace dump
{
namespace sbe
{
template <typename T>
using ServerObject = typename sdbusplus::server::object::object<T>;

using EntryIfaces = sdbusplus::server::object::object<
    sdbusplus::com::ibm::Dump::Entry::server::SBE>;

class Manager;

/** @class Entry
 *  @brief SBE Dump Entry implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Entry DBus API
 */
class Entry :
    virtual public EntryIfaces,
    virtual public phosphor::dump::bmc_stored::Entry
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
     *  @param[in] fileSize - Dump file size in bytes.
     *  @param[in] file - Name of dump file.
     *  @param[in] status - status of the dump.
     *  @param[in] parent - The dump entry's parent.
     */
    Entry(sdbusplus::bus::bus& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t fileSize,
          const std::filesystem::path& file,
          phosphor::dump::OperationStatus status,
          phosphor::dump::Manager& parent) :
        EntryIfaces(bus, objPath.c_str(), true),
        phosphor::dump::bmc_stored::Entry(bus, objPath.c_str(), dumpId,
                                          timeStamp, fileSize, file, status,
                                          parent)
    {
        // Emit deferred signal.
        this->openpower::dump::sbe::EntryIfaces::emit_object_added();
    }
};

} // namespace sbe
} // namespace dump
} // namespace openpower
