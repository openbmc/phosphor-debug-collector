#pragma once

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
    DumpEntryHelper(BaseEntry& dumpEntry, phosphor::dump::DumpTypes dumpType) :
        dumpEntry(dumpEntry), dumpType(dumpType)
    {}

    /** @brief Delete this d-bus object.
     */
    void delete_()
    {
        // Offload URI will be set during dump offload
        // Prevent delete when offload is in progress
        if ((!offloadUri().empty()) && (phosphor::dump::isHostRunning()))
        {
            lg2::error("Dump offload is in progress id: {DUMP_ID} "
                       "srcdumpid: {SRC_DUMP_ID}",
                       "DUMP_ID", dumpId, "SRC_DUMP_ID", srcDumpID);
            elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
        }

        lg2::info("System dump delete id: {DUMP_ID} srcdumpid: {SRC_DUMP_ID}",
                  "DUMP_ID", dumpId, "SRC_DUMP_ID", srcDumpID);

        // Remove host system dump when host is up by using source dump id
        // which is present in system dump entry dbus object as a property.
        if ((phosphor::dump::isHostRunning()) &&
            (srcDumpID != INVALID_SOURCE_ID))
        {
            try
            {
                dynamic_cast<Manager&>(dumpEntry.getParent())
                    .getHostTransport()
                    ->requestDelete(srcDumpID, dumpType);
            }
            catch (const std::exception& e)
            {
                lg2::error("Error deleting dump from host id: {DUMP_ID} "
                           "host id: {SRC_DUMP_ID} error: {ERROR}",
                           "DUMP_ID", dumpId, "SRC_DUMP_ID", srcDumpID, "ERROR",
                           e);
                elog<sdbusplus::xyz::openbmc_project::Common::Error::
                         Unavailable>();
            }
        }
    }

    /**
     * @brief Initiate the dump offload process
     * @param id - Dump ID
     * @param uri - URI to offload the dump to
     */
    void initiateOffload(uint32_t id, std::string uri)
    {
        dynamic_cast<Manager&>(dumpEntry.getParent())
            .getHostTransport()
            ->requestOffload(sourceDumpId());
    }

    /** @brief Method to get the file handle of the dump
     *  @returns A Unix file descriptor to the dump file
     *  @throws sdbusplus::xyz::openbmc_project::Common::File::Error::Open on
     *  failure to open the file
     *  @throws sdbusplus::xyz::openbmc_project::Common::Error::Unavailable if
     *  the file string is empty
     */
    sdbusplus::message::unix_fd getFileHandle(uint32_t)
    {
        using namespace phosphor::logging;

        lg2::error("This function is unavailable on this type of dump.");
        elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
        return 0;
    }

  private:
    /* @brief Dump entry object*/
    BaseEntry& dumpEntry;

    /* @brief Type of dump */
    phosphor::dump::DumpTypes dumpType;
};

} // namespace host
} // namespace dump
} // namespace openpower
