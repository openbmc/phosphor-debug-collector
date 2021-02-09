#pragma once

#include "com/ibm/Dump/Entry/Hostboot/server.hpp"
#include "local_dump_entry.hpp"

#include <filesystem>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

namespace openpower
{
namespace dump
{
namespace hostboot
{
template <typename T>
using ServerObject = typename sdbusplus::server::object::object<T>;

using EntryIfaces = sdbusplus::server::object::object<
    sdbusplus::com::ibm::Dump::Entry::server::Hostboot>;

// class Manager;

/** @class Entry
 *  @brief Hostboot Dump Entry implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Entry DBus API
 */
class Entry : virtual public EntryIfaces,
              virtual public phosphor::dump::local::Entry
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
          uint64_t timeStamp, uint64_t fileSize, const fs::path& file,
          phosphor::dump::OperationStatus status,
          phosphor::dump::Manager& parent) :
        EntryIfaces(bus, objPath.c_str(), true),
        phosphor::dump::local::Entry(bus, objPath.c_str(), dumpId, timeStamp,
                                     fileSize, file, status, parent)
    {
    }
};

} // namespace hostboot
} // namespace dump
} // namespace openpower
