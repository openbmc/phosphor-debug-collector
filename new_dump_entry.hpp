#pragma once

#include "base_dump_entry.hpp"
#include "dump_offload.hpp"

#include <fcntl.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>
#include <xyz/openbmc_project/Common/File/error.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <cstring>
#include <filesystem>

namespace phosphor
{
namespace dump
{
class Manager;
namespace new_
{

template <typename T>
using DumpEntryIface = sdbusplus::server::object_t<T>;

using OperationStatus =
    sdbusplus::xyz::openbmc_project::Common::server::Progress::OperationStatus;

using OriginatorTypes = sdbusplus::xyz::openbmc_project::Common::server::
    OriginatedBy::OriginatorTypes;

/** @class Entry
 *  @brief Concrete derived class for Dump Entry.
 *  @details This class provides a templated concrete implementation for the
 *  xyz.openbmc_project.Dump.Entry DBus API, derived from the BaseEntry class.
 *
 *  The Entry class is intended to be instantiated with a specific
 *  DBus interface type as the template parameter T, providing functionality
 *  specific to the corresponding dump type.
 *
 */
template <typename T, typename U>
class Entry : public BaseEntry, public DumpEntryIface<T>
{
    friend U;

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
     *  @param[in] dumpSize - Dump file size in bytes.
     *  @param[in] file - Name of dump file.
     *  @param[in] originId - Id of the originator of the dump
     *  @param[in] originType - Originator type
     *  @param[in] parent - The dump entry's parent.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t dumpSize,
          const std::filesystem::path& file, OperationStatus dumpStatus,
          std::string originId, OriginatorTypes originType,
          phosphor::dump::Manager& parent) :
        BaseEntry(bus, objPath, dumpId, timeStamp, dumpSize, file, dumpStatus,
                  originId, originType, parent),
        DumpEntryIface<T>(bus, objPath.c_str(),
                          DumpEntryIface<T>::action::emit_no_signals),
        helper(*this, file)
    {
        this->phosphor::dump::new_::DumpEntryIface<T>::emit_object_added();
    };

    /** @brief Delete this d-bus object.
     */
    void delete_() override
    {
        helper.delete_();
        BaseEntry::delete_();
    }

    /** @brief Method to initiate the offload of dump
     *  @param[in] uri - URI to offload dump
     */
    void initiateOffload(std::string uri) override
    {
        BaseEntry::initiateOffload(uri);
        helper.initiateOffload(id, uri);
        offloaded(true);
    }

    /** @brief Method to get the file handle of the dump
     *  @returns A Unix file descriptor to the dump file
     *  @throws sdbusplus::xyz::openbmc_project::Common::File::Error::Open on
     *  failure to open the file
     *  @throws sdbusplus::xyz::openbmc_project::Common::Error::Unavailable if
     *  the file string is empty
     */
    sdbusplus::message::unix_fd getFileHandle() override
    {
        return helper.getFileHandle(id);
    }

    /** @brief Method to update an existing dump entry, once the dump creation
     *  is completed this function will be used to update the entry which got
     *  created during the dump request.
     *  @param[in] timeStamp - Dump creation timestamp
     *  @param[in] fileSize - Dump file size in bytes.
     *  @param[in] file - Name of dump file.
     */
    void markComplete(uint64_t timeStamp, uint64_t fileSize,
                      const std::filesystem::path& filePath)
    {
        elapsed(timeStamp);
        size(fileSize);
        // TODO: Handled dump failed case with #ibm-openbmc/2808
        status(OperationStatus::Completed);
        heleper.setFile(filePath);
        // TODO: serialization of this property will be handled with
        // #ibm-openbmc/2597
        completedTime(timeStamp);
    }

  private:
    U helper;
};

} // namespace new_
} // namespace dump
} // namespace phosphor
