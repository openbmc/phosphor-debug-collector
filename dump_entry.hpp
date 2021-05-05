#pragma once

#include "xyz/openbmc_project/Common/FilePath/server.hpp"
#include "xyz/openbmc_project/Common/OriginatedBy/server.hpp"
#include "xyz/openbmc_project/Common/Progress/server.hpp"
#include "xyz/openbmc_project/Dump/Entry/server.hpp"
#include "xyz/openbmc_project/Object/Delete/server.hpp"
#include "xyz/openbmc_project/Time/EpochTime/server.hpp"

#include <fcntl.h>
#include <cstring>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include "base_dump_entry.hpp"
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Common/File/error.hpp>
#include <xyz/openbmc_project/Common/error.hpp>


#include <filesystem>

namespace phosphor
{
namespace dump
{

template <typename T>
using DumpEntryIface = sdbusplus::server::object_t<T>;

using OperationStatus =
    sdbusplus::xyz::openbmc_project::Common::server::Progress::OperationStatus;

using OriginatorTypes = sdbusplus::xyz::openbmc_project::Common::server::
    OriginatedBy::OriginatorTypes;

class Manager;

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
template <typename T>
class Entry : public BaseEntry, public DumpEntryIface<T>
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
     *  @param[in] dumpSize - Dump file size in bytes.
     *  @param[in] file - Name of dump file.
     *  @param[in] originId - Id of the originator of the dump
     *  @param[in] originType - Originator type
     *  @param[in] parent - The dump entry's parent.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t dumpSize,
          const std::filesystem::path& file, OperationStatus dumpStatus,
          std::string originId, OriginatorTypes originType, Manager& parent) :
        BaseEntry(bus, objPath, dumpId, timeStamp, dumpSize, file, dumpStatus, originId,
                  originType, parent),
            DumpEntryIface<T>(bus, objPath.c_str(),
                        DumpEntryIface<T>::action::emit_no_signals)
    {
        this->phosphor::dump::DumpEntryIface<T>::emit_object_added();
    };

    /** @brief Delete this d-bus object.
     */
    void delete_() override
    {
        // Delete Dump file from Permanent location
        try
        {
            std::filesystem::remove_all(file.parent_path());
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            // Log Error message and continue
            lg2::error(
                "Failed to delete dump file: {DUMP_FILE}, errormsg: {ERROR_MSG}",
                "DUMP_FILE", file, "ERROR_MSG", e.what());
        }

        // Remove Dump entry D-bus object
	BaseEntry::delete_();
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
    using namespace phosphor::logging;
    using namespace sdbusplus::xyz::openbmc_project::Common::File::Error;
    using metadata = xyz::openbmc_project::Common::File::Open;
    if (file.empty())
    {
        lg2::error("Failed to get file handle: File path is empty.");
        elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
    }

    if (fdCloseEventSource)
    {
        // Return the existing file descriptor
        return fdCloseEventSource->first;
    }

    int fd = open(file.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd == -1)
    {
        auto err = errno;
        lg2::error("Failed to open dump file: id: {ID} error: {ERRNO}", "ID",
                   id, "ERRNO", std::strerror(errno));
        elog<Open>(metadata::ERRNO(err), metadata::PATH(file.c_str()));
    }

    // Create a new Defer event source for closing this fd
    sdeventplus::Event event = sdeventplus::Event::get_default();
    auto eventSource = std::make_unique<sdeventplus::source::Defer>(
        event, [this](auto& /*source*/) { closeFD(); });

    // Store the file descriptor and event source in the optional pair
    fdCloseEventSource = std::make_pair(fd, std::move(eventSource));

    return fd;
}


  private:
    /** @brief Closes the file descriptor and removes the corresponding event
     *  source.
     *
     */
    void closeFD()
    {
        if (fdCloseEventSource)
        {
            close(fdCloseEventSource->first);
            fdCloseEventSource.reset();
        }
    }

    /* @brief A pair of file descriptor and corresponding event source. */
    std::optional<std::pair<int, std::unique_ptr<sdeventplus::source::Defer>>>
        fdCloseEventSource;
};

} // namespace dump
} // namespace phosphor
