#pragma once

#include "dump_entry.hpp"
#include "xyz/openbmc_project/Dump/Entry/BMC/server.hpp"
#include "xyz/openbmc_project/Dump/Entry/server.hpp"
#include "xyz/openbmc_project/Object/Delete/server.hpp"
#include "xyz/openbmc_project/Time/EpochTime/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

#include <filesystem>

namespace phosphor
{
namespace dump
{
namespace bmc
{
template <typename T>
using ServerObject = typename sdbusplus::server::object_t<T>;

using EntryIfaces = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Dump::Entry::server::BMC>;

using originatorTypes = sdbusplus::xyz::openbmc_project::Common::server::
    OriginatedBy::OriginatorTypes;

class Manager;

/** @class Entry
 *  @brief OpenBMC Dump Entry implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Entry DBus API
 */
class Entry : virtual public phosphor::dump::Entry, virtual public EntryIfaces
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
     *  @param[in] file - Absolute path to the dump file.
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
        phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, fileSize,
                              file, status, originatorId, originatorType,
                              parent),
        EntryIfaces(bus, objPath.c_str(), EntryIfaces::action::defer_emit)
    {
        // Emit deferred signal.
        this->phosphor::dump::bmc::EntryIfaces::emit_object_added();
    }

    /** @brief Delete this d-bus object.
     */
    void delete_() override;

    /** @brief Method to initiate the offload of dump
     *  @param[in] uri - URI to offload dump
     */
    void initiateOffload(std::string uri) override;

    /** @brief Method to update an existing dump entry, once the dump creation
     *  is completed this function will be used to update the entry which got
     *  created during the dump request.
     *  @param[in] timeStamp - Dump creation timestamp
     *  @param[in] fileSize - Dump file size in bytes.
     *  @param[in] file - Name of dump file.
     */
    void update(uint64_t timeStamp, uint64_t fileSize,
                const std::filesystem::path& filePath)
    {
        elapsed(timeStamp);
        size(fileSize);
        // TODO: Handled dump failed case with #ibm-openbmc/2808
        status(OperationStatus::Completed);
        file = filePath;
        // TODO: serialization of this property will be handled with
        // #ibm-openbmc/2597
        completedTime(timeStamp);
        serialize();
    }

    /**
     * @brief Update dump entry attributes from the file name.
     *
     * @param[in] dumpPath - The path to the dump directory.
     */
    void updateFromFile(const std::filesystem::path& dumpPath);

    /**
     * @brief Deserialize and create an entry
     * @param[in] bus - Bus to attach to.
     * @param[in] id - Dump id.
     * @param[in] objPath - Object path to attach to.
     * @param[in] filePath - Path to the dump file.
     * @param[in] parent - The dump entry's parent.
     * @return A unique pointer to the created entry.
     */
    static std::unique_ptr<Entry> deserializeEntry(
        sdbusplus::bus_t& bus, uint32_t id, const std::string& objPath,
        const std::filesystem::path& filePath, phosphor::dump::Manager& parent)
    {
        try
        {
            auto entry = std::unique_ptr<Entry>(
                new Entry(bus, objPath, id, filePath, parent));
            entry->updateFromFile(filePath);
            entry->deserialize(filePath.parent_path());
            entry->emitSignal();
            return entry;
        }
        catch (const std::exception& e)
        {
            lg2::error(
                "Dump deserialization failed for path: {PATH}, error: {ERROR}",
                "PATH", filePath, "ERROR", e.what());
            return nullptr;
        }
    }

  private:
    /**
     *  @brief A minimal private constructor for the Dump Entry Object
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Object path to attach to
     *  @param[in] dumpId - Dump id.
     *  @param[in] file - Absolute path to the dump file.
     *  @param[in] parent - The dump entry's parent.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          const std::filesystem::path& file, phosphor::dump::Manager& parent) :
        phosphor::dump::Entry(bus, objPath.c_str(), dumpId, 0, 0, file,
                              OperationStatus::InProgress, "",
                              originatorTypes::Internal, parent),
        EntryIfaces(bus, objPath.c_str(), EntryIfaces::action::defer_emit)
    {}

    /**
     * @brief Emit object added signal
     */
    void emitSignal()
    {
        this->phosphor::dump::bmc::EntryIfaces::emit_object_added();
    }
};

} // namespace bmc
} // namespace dump
} // namespace phosphor
