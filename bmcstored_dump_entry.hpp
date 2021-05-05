#pragma once

#include "dump_entry.hpp"
#include "dump_offload.hpp"
#include "xyz/openbmc_project/Common/FilePath/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

#include <filesystem>

namespace phosphor
{
namespace dump
{
namespace bmc_stored
{

template <typename T>
using DumpEntryIface = sdbusplus::server::object_t<T>;

class Manager;

/** @class Entry
 *  @brief Entry sase class for all dumps get stored on BMC.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Entry DBus API
 */
template <typename T>
class Entry : public phosphor::dump::Entry, public DumpEntryIface<T>
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
     *  @param[in] originId - Id of the originator of the dump
     *  @param[in] originType - Originator type
     *  @param[in] parent - The dump entry's parent.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t fileSize,
          const std::filesystem::path& file,
          phosphor::dump::OperationStatus status, std::string originId,
          originatorTypes originType, phosphor::dump::Manager& parent) :
        phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, fileSize,
                              file, status, originId, originType, parent),
        DumpEntryIface<T>(bus, objPath.c_str())
    {
        this->phosphor::dump::bmc_stored::DumpEntryIface<
            T>::emit_object_added();
    }

    /** @brief Delete this d-bus object.
     */
    void delete_() override
    {
        // Delete Dump file from Permanent location
        try
        {
            std::filesystem::remove_all(
                std::filesystem::path(path()).parent_path());
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            // Log Error message and continue
            lg2::error(
                "Failed to delete dump file: {DUMP_FILE}, errormsg: {ERROR_MSG}",
                "DUMP_FILE", path(), "ERROR_MSG", e.what());
        }

        // Remove Dump entry D-bus object
        phosphor::dump::Entry::delete_();
    }

    /** @brief Method to initiate the offload of dump
     *  @param[in] uri - URI to offload dump
     */
    void initiateOffload(std::string uri) override
    {
        phosphor::dump::offload::requestOffload(path(), id, uri);
        offloaded(true);
    }
};

} // namespace bmc_stored
} // namespace dump
} // namespace phosphor
