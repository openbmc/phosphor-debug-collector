#pragma once

#include "dump_manager.hpp"
#include "faultlog_dump_entry.hpp"

#include <fmt/core.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

namespace phosphor
{
namespace dump
{
namespace faultlog
{

using namespace phosphor::logging;

using CreateIface = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Dump::server::Create>;

/** @class Manager
 *  @brief FaultLog Dump manager implementation.
 */
class Manager :
    virtual public CreateIface,
    virtual public phosphor::dump::Manager
{
  public:
    Manager() = delete;
    Manager(const Manager&) = default;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    virtual ~Manager() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] path - Path to attach at.
     *  @param[in] baseEntryPath - Base path for dump entry.
     *  @param[in] filePath - Path where the dumps are stored.
     */
    Manager(sdbusplus::bus_t& bus, const char* path,
            const std::string& baseEntryPath, const char* filePath) :
        CreateIface(bus, path),
        phosphor::dump::Manager(bus, path, baseEntryPath), dumpDir(filePath)
    {
        prevTimestamp = 0;
        std::error_code ec;

        std::filesystem::create_directory(FAULTLOG_DUMP_PATH, ec);

        if (ec)
        {
            log<level::ERR>(fmt::format("dump_manager_faultlog directory {} "
                                        "not created. error_code = {} ({})",
                                        FAULTLOG_DUMP_PATH, ec.value(),
                                        ec.message())
                                .c_str());
        }

        registerFaultLogMatches();
    }

    void restore() override
    {
        // TODO phosphor-debug-collector/issues/21: Restore fault log entries
        // after service restart
        log<level::INFO>("dump_manager_faultlog restore not implemented");
    }

    /** @brief  Delete all fault log entries and their corresponding fault log
     * dump files */
    void deleteAll() override;

    /** @brief Method to create a new fault log dump entry
     *  @param[in] params - Key-value pair input parameters
     *
     *  @return object_path - The path to the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(phosphor::dump::DumpCreateParams params) override;

  private:
    static constexpr uint32_t MAX_NUM_FAULT_LOG_ENTRIES =
        MAX_TOTAL_FAULT_LOG_ENTRIES - MAX_NUM_SAVED_CRASHDUMP_ENTRIES -
        MAX_NUM_SAVED_CPER_LOG_ENTRIES;

    /** @brief Map of saved CPER log entry dbus objects based on entry id */
    std::map<uint32_t, std::unique_ptr<phosphor::dump::Entry>>
        savedCperLogEntries;

    /** @brief Map of saved crashdump entry dbus objects based on entry id */
    std::map<uint32_t, std::unique_ptr<phosphor::dump::Entry>>
        savedCrashdumpEntries;

    /** @brief Path to the dump file*/
    std::string dumpDir;

    /** @brief D-Bus match for crashdump completion signal */
    std::unique_ptr<sdbusplus::bus::match_t> crashdumpMatch;

    /** @brief D-Bus match for CPER log added signal */
    std::unique_ptr<sdbusplus::bus::match_t> cperLogMatch;
    /** @brief D-Bus match for old CPER log added signal (temporary, to be
     * removed after OpenBMC transitions to sending the new signal)*/
    std::unique_ptr<sdbusplus::bus::match_t> cperLogMatchOld;

    /** @brief The last generated timestamp (microseconds since epoch) */
    uint64_t prevTimestamp;

    /** @brief Register D-Bus match rules to detect fault events */
    void registerFaultLogMatches();
    /** @brief Register D-Bus match rules to detect new crashdumps */
    void registerCrashdumpMatch();
    /** @brief Register D-Bus match rules to detect CPER logs */
    void registerCperLogMatch();

    /** @brief Get and check parameters for createDump() function (throws
     * exception on error)
     *  @param[in] params - Key-value pair input parameters
     *  @param[out] entryType - Log entry type (corresponding to type of data in
     * primary fault data log)
     *  @param[out] primaryLogIdStr - Id of primary fault data log
     */
    void getAndCheckCreateDumpParams(
        const phosphor::dump::DumpCreateParams& params,
        FaultDataType& entryType, std::string& primaryLogIdStr);

    /** @brief Generate the current timestamp, adjusting as needed to ensure an
     * increase compared to the last fault log entry's timestamp
     *
     *  @return timestamp - microseconds since epoch
     */
    uint64_t generateTimestamp();

    /** @brief Save earliest fault log entry (if it qualifies to be saved) and
     * remove it from the main fault log entries map.
     *
     *  More specifically, move the earliest entry from the fault log
     *  entries map to the saved entries map based on its type. Before
     *  moving it, this function checks (1) whether a saved entries map
     *  exists for the entry type, and if so, then (2) whether the
     *  saved entries map is already full. If the entry can't be saved,
     *  then it's simply deleted from the main fault log entries map.
     */
    void saveEarliestEntry();
};

} // namespace faultlog
} // namespace dump
} // namespace phosphor
