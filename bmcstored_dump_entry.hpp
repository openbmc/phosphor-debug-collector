#pragma once

#include "dump_entry.hpp"
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
using ServerObject = typename sdbusplus::server::object::object<T>;
using FileIfaces = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Common::server::FilePath>;

class Manager;

/** @class Entry
 *  @brief Entry sase class for all dumps get stored on BMC.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Entry DBus API
 */
class Entry : public phosphor::dump::Entry, public FileIfaces
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
        phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, fileSize,
                              status, parent),
        FileIfaces(bus, objPath.c_str(), true)
    {
        path(file);
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
        path(filePath);
        // TODO: serialization of this property will be handled with
        // #ibm-openbmc/2597
        completedTime(timeStamp);
    }

  protected:
    /** @Dump file name */
    std::filesystem::path file;

    /** @brief Function to call dump offload method
     *  @return 0 if success
     */
    uint32_t downloadHelper();
};

} // namespace bmc_stored
} // namespace dump
} // namespace phosphor
