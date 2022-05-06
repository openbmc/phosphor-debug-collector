#pragma once

#include "dump-extensions/openpower-dumps/openpower_dumps_config.h"

#include "bmcstored_dump_entry.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

#include <filesystem>

namespace openpower
{
namespace dump
{
namespace hostdump
{

using originatorTypes = sdbusplus::xyz::openbmc_project::Common::server::
    OriginatedBy::OriginatorTypes;

template <typename T>
using ServerObject = typename sdbusplus::server::object_t<T>;

template <typename T>
using EntryIfaces = sdbusplus::server::object_t<T>;

/** @class Entry
 *  @brief Host Dump Entry implementation.
 *  @details A concrete implementation for the
 *  host dump type DBus API
 */
template <typename T>
class Entry :
    virtual public EntryIfaces<T>,
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
     *  @param[in] originatorId - Id of the originator of the dump
     *  @param[in] originatorType - Originator type
     *  @param[in] parent - The dump entry's parent.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t fileSize,
          const std::filesystem::path& file,
          phosphor::dump::OperationStatus status, std::string originatorId,
          originatorTypes originatorType, phosphor::dump::Manager& parent) :
        EntryIfaces<T>(bus, objPath.c_str(),
                       EntryIfaces<T>::action::emit_object_added),
        phosphor::dump::bmc_stored::Entry(bus, objPath.c_str(), dumpId,
                                          timeStamp, fileSize, file, status,
                                          originatorId, originatorType, parent)
    {
        // Emit deferred signal.
        this->openpower::dump::hostdump::EntryIfaces<T>::emit_object_added();
    }

    /** @brief Delete the dump and D-Bus object
     */
    void delete_() override
    {
        auto dumpId = id;
        auto dumpPath = path();
        phosphor::dump::bmc_stored::Entry::delete_();

        std::string dumpName;
        if (dumpId > SBE_DUMP_START_ID)
        {
            dumpName = "SBE Dump";
        }
        else if (dumpId > HOSTBOOT_DUMP_START_ID)
        {
            dumpName = "Hostboot Dump";
        }
        else if (dumpId > HARDWARE_DUMP_START_ID)
        {
            dumpName = "Hardware Dump";
        }
        else
        {
            dumpName = "Unknown Dump";
        }

        auto bus = sdbusplus::bus::new_default();
        // Log PEL for dump delete/offload
        phosphor::dump::createPEL(
            bus, dumpPath, dumpName, dumpId,
            "xyz.openbmc_project.Logging.Entry.Level.Informational",
            "xyz.openbmc_project.Dump.Error.Invalidate");
    }
};

} // namespace hostdump
} // namespace dump
} // namespace openpower
