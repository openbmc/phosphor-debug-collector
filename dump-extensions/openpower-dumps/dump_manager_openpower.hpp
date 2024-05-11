#pragma once

#include "dump_manager.hpp"
#include "dump_utils.hpp"

#include <com/ibm/Dump/Create/common.hpp>
#include <com/ibm/Dump/Notify/common.hpp>
#include <com/ibm/Dump/Notify/server.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

namespace openpower::dump
{

constexpr auto OP_BASE_ENTRY_PATH = "/xyz/openbmc_project/dump/opdump/entry";
constexpr auto OP_DUMP_OBJ_PATH = "/xyz/openbmc_project/dump/opdump";

using OpDumpIfaces = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Dump::server::Create,
    sdbusplus::com::ibm::Dump::server::Notify>;

using NotifyDumpTypes = sdbusplus::common::com::ibm::dump::Notify::DumpType;
using OpDumpTypes = sdbusplus::common::com::ibm::dump::Create::DumpType;
/** @class Manager
 *  @brief OpenPOWER Dump manager implementation.
 *  @details A concrete implementation for the com.ibm.Dump.Notify and
 *           xyz.openbmc_project.Dump.Create DBus APIs
 */
class Manager :
    virtual public OpDumpIfaces,
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
     *  @param[in] baseEntryPath - Base path of the dump entry.
     */
    Manager(sdbusplus::bus_t& bus, const char* path,
            const std::string& baseEntryPath) :
        OpDumpIfaces(bus, path),
        phosphor::dump::Manager(bus, path, baseEntryPath)
    {}

    void restore() override
    {
        // TODO #2597  Implement the restore to restore the dump entries
        // after the service restart.
    }

    /** @brief Notify the system dump manager about creation of a new dump.
     *  @param[in] dumpId - Id from the source of the dump.
     *  @param[in] size - Size of the dump.
     *  @param[in] type - Type of the dump being notified
     *  @param[in] token - Token identifying the specific dump
     */
    void notifyDump(uint32_t dumpId, uint64_t size, NotifyDumpTypes type,
                    [[maybe_unused]] uint32_t token) override;

    /** @brief Implementation for CreateDump
     *  Method to create a new system dump entry when user
     *  requests for a new system dump.
     *
     *  @return object_path - The path to the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(phosphor::dump::DumpCreateParams params) override;

    inline OpDumpTypes convertNotifyToCreateType(NotifyDumpTypes type)
    {
        using namespace phosphor::logging;
        using InvalidArgument =
            sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
        using Argument = xyz::openbmc_project::Common::InvalidArgument;
        switch (type)
        {
            case NotifyDumpTypes::System:
                return OpDumpTypes::System;
            case NotifyDumpTypes::Resource:
                return OpDumpTypes::Resource;
            default:
                lg2::error("An invalid type passed: {TYPE}", "TYPE", type);
                elog<InvalidArgument>(
                    Argument::ARGUMENT_NAME("TYPE"),
                    Argument::ARGUMENT_VALUE("INVALID INPUT"));
        }
    }
};

} // namespace openpower::dump
