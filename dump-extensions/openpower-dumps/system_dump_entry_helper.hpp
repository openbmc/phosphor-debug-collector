#pragma once

#include "base_dump_entry.hpp"
#include "com/ibm/Dump/Entry/Resource/server.hpp"
#include "dump_utils.hpp"
#include "host_transport_exts.hpp"
#include "op_dump_util.hpp"
#include "xyz/openbmc_project/Dump/Entry/System/server.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace openpower
{
namespace dump
{

class DumpEntryHelper
{
  public:
    DumpEntryHelper(phosphor::dump::BaseEntry& dumpEntry) : dumpEntry(dumpEntry)
    {}

    void initiateOffload(uint32_t id, uint32_t sourceDumpId,
                         const std::string& uri);

    /** @brief Method to update an existing dump entry
     *  @param[in] timeStamp - Dump creation timestamp
     *  @param[in] dumpSize - Dump size in bytes.
     *  @param[in] sourceId - DumpId provided by the source.
     */
    void markComplete(uint64_t timeStamp, uint64_t dumpSize, uint32_t sourceId);

    /**
     * @brief Delete host system dump and it entry dbus object
     */
    void delete_(uint32_t dumpId, uint32_t sourceDumpId, uint8_t transportId,
                 const std::string& dumpPathOffLoadUri);
    /** @brief Method to get the file handle of the dump
     *  @returns A Unix file descriptor to the dump file
     *  @throws sdbusplus::xyz::openbmc_project::Common::File::Error::Open on
     *  failure to open the file
     *  @throws sdbusplus::xyz::openbmc_project::Common::Error::Unavailable if
     *  the file string is empty
     */
    sdbusplus::message::unix_fd getFileHandle();

  private:
    phosphor::dump::BaseEntry& dumpEntry;
};

} // namespace dump
} // namespace openpower
