#pragma once

#include "base_dump_entry.hpp"

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

/** @class DumpEntryHelper
 */
class DumpEntryHelper
{
  public:
    DumpEntryHelper() = delete;
    DumpEntryHelper(const DumpEntryHelper&) = delete;
    DumpEntryHelper& operator=(const DumpEntryHelper&) = delete;
    DumpEntryHelper(DumpEntryHelper&&) = delete;
    DumpEntryHelper& operator=(DumpEntryHelper&&) = delete;
    ~DumpEntryHelper() = default;

    DumpEntryHelper(BaseEntry& dumpEntry) : dumpEntry(dumpEntry) {}

    /** @brief Delete this d-bus object.
     */
    void delete_(const std::filesystem::path& file)
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
        dumpEntry.BaseEntry::delete_();
    }

    /** @brief Method to get the file handle of the dump
     *  @returns A Unix file descriptor to the dump file
     *  @throws sdbusplus::xyz::openbmc_project::Common::File::Error::Open on
     *  failure to open the file
     *  @throws sdbusplus::xyz::openbmc_project::Common::Error::Unavailable if
     *  the file string is empty
     */
    sdbusplus::message::unix_fd getFileHandle(uint32_t dumpId, const std::filesystem::path& file)
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
            lg2::error("Failed to open dump file: id: {ID} error: {ERRNO}",
                       "ID", dumpId, "ERRNO", std::strerror(errno));
            elog<Open>(metadata::ERRNO(err),
                       metadata::PATH(file.c_str()));
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
    BaseEntry& dumpEntry;
};

} // namespace dump
} // namespace phosphor
