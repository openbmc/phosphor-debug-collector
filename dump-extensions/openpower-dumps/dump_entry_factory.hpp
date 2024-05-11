#pragma once

#include "dump_entry.hpp"
#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "op_dump_util.hpp"
#include "resource_dump_entry.hpp"
#include "system_dump_entry.hpp"

#include <com/ibm/Dump/Create/common.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

#include <filesystem>
#include <memory>

namespace openpower::dump
{

/**
 * @class DumpEntryFactory
 * @brief Factory class to create dump entries based on dump type.
 *        This class encapsulates the creation of dump entries.
 */
class DumpEntryFactory
{
  public:
    /**
     * @brief Constructs a dump entry factory.
     * @param[in] bus Reference to the D-Bus bus object.
     * @param[in] baseEntryPath Base object path for the dump entries.
     * @param[in] mgr Reference to the dump manager handling these dumps.
     */
    DumpEntryFactory(sdbusplus::bus::bus& bus, const std::string& baseEntryPath,
                     phosphor::dump::Manager& mgr) :
        bus(bus), baseEntryPath(baseEntryPath), mgr(mgr)
    {}

    /**
     * @brief Creates a dump entry based on provided parameters.
     * @param[in] id The unique identifier for the new dump entry.
     * @param[in] params Parameters defining the dump creation specifics.
     * @return A unique pointer to a newly created dump entry, or nullptr if
     * creation fails.
     */
    std::unique_ptr<phosphor::dump::Entry> createEntry(
        uint32_t id, phosphor::dump::DumpCreateParams& params);

  private:
    /**
     * @brief Creates a system dump entry.
     * @param[in] id The unique identifier for the system dump entry.
     * @param[in] objPath D-Bus entry path for the dump entry.
     * @param[in] timeStamp Timestamp marking the creation time of the dump.
     * @param[in] dumpParams Parameters specific to the dump being created.
     * @return A unique pointer to a newly created system dump entry.
     */
    std::unique_ptr<phosphor::dump::Entry> createSystemDumpEntry(
        uint32_t id, std::filesystem::path& objPath, uint64_t timeStamp,
        const DumpParameters& dumpParams);

    /**
     * @brief Retrieves the dump ID prefix based on the dump type.
     * @param[in] dumpType Type of the dump (system, resource, etc.).
     * @return The prefix to be used for the dump ID.
     */
    uint32_t getDumpIdPrefix(OpDumpTypes dumpType)
    {
        switch (dumpType)
        {
            case OpDumpTypes::System:
                return 0x00000000;
            default:
                lg2::error("unsupported {TYPE}", "TYPE", dumpType);
        }
        return 0xFF;
    }

    /** @brief sdbusplus DBus bus connection. */
    sdbusplus::bus_t& bus;

    /** @brief Base D-Bus path for dump entries. */
    const std::string& baseEntryPath;

    /** @brief Reference to the managing object for dumps.*/
    phosphor::dump::Manager& mgr;
};

} // namespace openpower::dump
