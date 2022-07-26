#pragma once

#include "dump_entry.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Dump/Entry/FaultLog/server.hpp>
#include <xyz/openbmc_project/Dump/Entry/server.hpp>
#include <xyz/openbmc_project/Object/Delete/server.hpp>
#include <xyz/openbmc_project/Time/EpochTime/server.hpp>

#include <filesystem>

namespace phosphor
{
namespace dump
{
namespace faultlog
{
template <typename T>
using ServerObject = typename sdbusplus::server::object_t<T>;

using EntryIfaces = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Dump::Entry::server::FaultLog>;

using FaultDataType = sdbusplus::xyz::openbmc_project::Dump::Entry::server::
    FaultLog::FaultDataType;

class Manager;

/** @class Entry
 *  @brief OpenBMC Fault Log Dump Entry implementation.
 */
class Entry : virtual public EntryIfaces, virtual public phosphor::dump::Entry
{
    friend class Manager;

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
     *             in microseconds since the epoch.
     *  @param[in] fileSize - Dump file size in bytes.
     *  @param[in] file - Full path of dump file.
     *  @param[in] status - status of the dump.
     *  @param[in] originatorId - Id of the originator of the dump
     *  @param[in] originatorType - Originator type
     *  @param[in] parent - The dump entry's parent.
     */
    Entry(
        sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
        uint64_t timeStamp, uint64_t fileSize,
        const std::filesystem::path& file,
        phosphor::dump::OperationStatus status, std::string originatorId,
        originatorTypes originatorType, FaultDataType entryType,
        const std::string& primaryLogIdStr, phosphor::dump::Manager& parent,
        std::map<uint32_t, std::unique_ptr<phosphor::dump::Entry>>* parentMap) :
        EntryIfaces(bus, objPath.c_str(), EntryIfaces::action::defer_emit),
        phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, fileSize,
                              status, originatorId, originatorType, parent),
        parentMap(parentMap), file(file)
    {
        type(entryType);
        primaryLogId(primaryLogIdStr);

        // Emit deferred signal.
        this->phosphor::dump::faultlog::EntryIfaces::emit_object_added();
    }

    /** @brief Delete this d-bus object.
     */
    void delete_() override;

  private:
    /** @brief Fault log map containing this entry (e.g. the main fault log map
     * or a saved entries map) */
    std::map<uint32_t, std::unique_ptr<phosphor::dump::Entry>>* parentMap;

    /** @brief Dump file path */
    std::filesystem::path file;
};

} // namespace faultlog
} // namespace dump
} // namespace phosphor
