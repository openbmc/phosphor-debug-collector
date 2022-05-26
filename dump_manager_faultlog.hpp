#pragma once

#include "dump_manager.hpp"

#include <fmt/core.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <sdbusplus/bus.hpp>
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
    }

    /** @brief Perform any post restore operations after claiming
     *  the bus name. Any new D-Bus dump objects created will be
     *  notified to the subscribers.
     */
    void checkAndInitialize() override
    {}

    void restore() override
    {
        // TODO phosphor-debug-collector/issues/21: Restore fault log entries
        // after service restart
        log<level::INFO>("dump_manager_faultlog restore not implemented");
    }

    /** @brief Method to create a new fault log dump entry
     *  @param[in] params - Key-value pair input parameters
     *
     *  @return object_path - The path to the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(phosphor::dump::DumpCreateParams params) override;

  private:
    /** @brief Path to the dump file*/
    std::string dumpDir;
};

} // namespace faultlog
} // namespace dump
} // namespace phosphor
