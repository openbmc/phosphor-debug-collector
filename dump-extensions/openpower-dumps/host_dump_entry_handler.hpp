#pragma once

#include "base_dump_entry.hpp"
#include "dump_utils.hpp"
#include "host_transport_exts.hpp"

namespace openpower
{
namespace dump
{
namespace host
{

/**
 * @class DumpEntryHelper
 * @brief A helper class for handling Dump Entries
 *
 * This class provides functionalities to manage a dump file stored in the host
 * It allows deleting a Dump Entry, initiating an offload, and handling
 * the file descriptor associated with the dump file.
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

    /**
     * @brief Constructor
     * @param dumpEntry - Reference to BaseEntry object
     * @param file - Path to the dump file
     */
    DumpEntryHelper(phosphor::dump::host::HostTransport& hostTransport,
                    phosphor::dump::DumpTypes dumpType) :
        hostTransport(hostTransport),
        dumpType(dumpType)
    {}

    /** @brief Delete this d-bus object.
     */
    void delete_(uint32_t dumpId, uint32_t srcDumpId, const std::string uri);

    /**
     * @brief Initiate the dump offload process
     * @param id - Dump ID
     * @param uri - URI to offload the dump to
     */
    void initiateOffload(uint32_t srcDumpId);

    /** @brief Method to get the file handle of the dump
     *  @returns A Unix file descriptor to the dump file
     *  @throws sdbusplus::xyz::openbmc_project::Common::File::Error::Open on
     *  failure to open the file
     *  @throws sdbusplus::xyz::openbmc_project::Common::Error::Unavailable if
     *  the file string is empty
     */
    sdbusplus::message::unix_fd getFileHandle(uint32_t);

  private:
    /* @brief Dump entry object*/
    phosphor::dump::host::HostTransport& hostTransport;

    /* @brief Type of dump */
    phosphor::dump::DumpTypes dumpType;
};

} // namespace host
} // namespace dump
} // namespace openpower
