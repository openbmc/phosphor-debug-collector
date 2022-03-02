#pragma once

#include "dump_manager.hpp"
#include "dump_utils.hpp"

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

// constexpr uint32_t INVALID_SOURCE_ID = 0xFFFFFFFF;
using CreateIface = sdbusplus::server::object::object<
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
     *  @param[in] event - Dump manager sd_event loop.
     *  @param[in] path - Path to attach at.
     *  @param[in] baseEntryPath - Base path for dump entry.
     *  @param[in] filePath - Path where the dumps are stored.
     */
    Manager(sdbusplus::bus::bus& bus, const EventPtr& event, const char* path,
            const std::string& baseEntryPath, const char* filePath) :
        CreateIface(bus, path),
        phosphor::dump::Manager(bus, path, baseEntryPath),
        eventLoop(event.get()), dumpDir(filePath)
    {
        numSavedEntries = 0;
        currFaultLogIdx = 0;

        faultLogEntries.reserve(MAX_FAULT_LOG_ENTRIES);
        for (uint32_t i = 0; i < MAX_FAULT_LOG_ENTRIES; i++)
        {
            faultLogEntries.push_back(nullptr);
        }

        savedEntries.reserve(MAX_SAVED_ENTRIES);
        for (uint32_t i = 0; i < MAX_SAVED_ENTRIES; i++)
        {
            savedEntries.push_back(nullptr);
        }
    }

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] event - Dump manager sd_event loop.
     *  @param[in] path - Path to attach at.
     *  @param[in] baseEntryPath - Base path of the dump entry.
     */
    Manager(sdbusplus::bus::bus& bus, const char* path,
            const std::string& baseEntryPath) :
        CreateIface(bus, path),
        phosphor::dump::Manager(bus, path, baseEntryPath)
    {}

    /** @brief Initialization including registering D-Bus matches
     */
    void init();

    void restore() override
    {
        // Fault log entries will not be restored
        log<level::INFO>("dump_manager_faultlog restore not implemented");
    }

    /** @brief Implementation for CreateDump
     *  Method to create a new system dump entry when user
     *  requests for a new system dump.
     *
     *  @return object_path - The path to the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(phosphor::dump::DumpCreateParams params) override;

    /** @brief sd_event_add_child callback
     *
     *  @param[in] s - event source
     *  @param[in] si - signal info
     *  @param[in] userdata - pointer to Watch object
     *
     *  @returns 0 on success, -1 on fail
     */
    static int callback(sd_event_source*, const siginfo_t*, void*)
    {
        log<level::INFO>("sd_event_add_child callback!");

        // No specific action required in
        // the sd_event_add_child callback.
        return 0;
    }

  protected:
    std::vector<std::unique_ptr<Entry>> faultLogEntries;
    std::vector<std::unique_ptr<Entry>> savedEntries;

    const uint32_t MAX_FAULT_LOG_ENTRIES = 7;
    const uint32_t MAX_SAVED_ENTRIES = 3;
    const uint32_t MAX_TOTAL_ENTRIES =
        MAX_FAULT_LOG_ENTRIES + MAX_SAVED_ENTRIES;
    uint32_t numSavedEntries;
    uint32_t currFaultLogIdx;

  private:
    /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

    /** @brief Path to the dump file*/
    std::string dumpDir;

    /** @brief D-Bus match for crashdump completion signal*/
    std::unique_ptr<sdbusplus::bus::match::match> crashdumpMatch;
};

} // namespace faultlog
} // namespace dump
} // namespace phosphor
