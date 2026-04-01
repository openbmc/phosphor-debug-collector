#pragma once

#include "dump_entry.hpp"
#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "op_dump_consts.hpp"
#include "op_dump_util.hpp"
#include "openpower_dump_entry.hpp"
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

    /**
     * @brief Handles the update or creation of a dump entry based on type.
     *
     * @param type The type of the dump as defined by OpDumpTypes.
     * @param sourceDumpId The source ID associated with the dump, used for
     * identification.
     * @param size The size of the dump in bytes.
     * @param id The intended ID for the new dump entry.
     * @param entries A map of existing dump entries indexed by their IDs.
     *
     * @return An optional containing a unique pointer to the dump entry if a
     * new dump entry is created. Returns std::nullopt if new entry is not
     * created
     */
    std::optional<std::unique_ptr<phosphor::dump::Entry>>
        createOrUpdateHostEntry(
            OpDumpTypes type, uint64_t sourceDumpId, uint64_t size, uint32_t id,
            uint32_t token,
            const std::map<uint32_t, std::unique_ptr<phosphor::dump::Entry>>&
                entries);

    std::unique_ptr<phosphor::dump::Entry> createEntryWithDefaults(
        uint32_t id, const std::filesystem::path& objPath);

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
     * @brief Creates a resource dump entry.
     * @param[in] id The unique identifier for the system dump entry.
     * @param[in] objPath D-Bus entry path for the dump entry.
     * @param[in] timeStamp Timestamp marking the creation time of the dump.
     * @param[in] dumpParams Parameters specific to the dump being created.
     * @return A unique pointer to a newly created resource dump entry.
     */
    std::unique_ptr<phosphor::dump::Entry> createResourceDumpEntry(
        uint32_t id, std::filesystem::path& objPath, uint64_t timeStamp,
        bool createSysDump, const DumpParameters& dumpParams);

    /**
     * @brief Creates a hostboot dump entry.
     * @param[in] id The unique identifier for the system dump entry.
     * @param[in] objPath D-Bus entry path for the dump entry.
     * @param[in] timeStamp Timestamp marking the creation time of the dump.
     * @param[in] dumpParams Parameters specific to the dump being created.
     * @return A unique pointer to a newly created hostboot dump entry.
     */
    std::unique_ptr<phosphor::dump::Entry> createHostbootDumpEntry(
        uint32_t id, std::filesystem::path& objPath, uint64_t timeStamp,
        const DumpParameters& dumpParams);

    /**
     * @brief Creates a hardware dump entry.
     * @param[in] id The unique identifier for the system dump entry.
     * @param[in] objPath D-Bus entry path for the dump entry.
     * @param[in] timeStamp Timestamp marking the creation time of the dump.
     * @param[in] dumpParams Parameters specific to the dump being created.
     * @return A unique pointer to a newly created hardware dump entry.
     */
    std::unique_ptr<phosphor::dump::Entry> createHardwareDumpEntry(
        uint32_t id, std::filesystem::path& objPath, uint64_t timeStamp,
        const DumpParameters& dumpParams);

    /**
     * @brief Creates a SBE dump entry.
     * @param[in] id The unique identifier for the system dump entry.
     * @param[in] objPath D-Bus entry path for the dump entry.
     * @param[in] timeStamp Timestamp marking the creation time of the dump.
     * @param[in] dumpParams Parameters specific to the dump being created.
     * @return A unique pointer to a newly created SBE dump entry.
     */
    std::unique_ptr<phosphor::dump::Entry> createSBEDumpEntry(
        uint32_t id, std::filesystem::path& objPath, uint64_t timeStamp,
        const DumpParameters& dumpParams);

    /**
     * @brief Update or create a new host dump entry.
     *
     * @tparam T The specific type of dump entry, must be derived from
     *         phosphor::dump::Entry.
     * @param dumpType The type of the dump as defined by OpDumpTypes.
     * @param srcDumpId The source ID associated with the dump.
     * @param size The size of the dump.
     * @param id The intended ID for the new dump entry if needed.
     * @param entries A map of existing dump entries indexed by their IDs.
     *
     * @return An optional containing a unique pointer to the new dump entry.
     *         Returns std::nullopt if no creation is needed.
     *
     * @note The function asserts at compile time that T is derived from
     *       phosphor::dump::Entry.
     */
    template <typename T>
    std::optional<std::unique_ptr<phosphor::dump::Entry>> createOrUpdate(
        OpDumpTypes dumpType, uint64_t srcDumpId, uint64_t size, uint32_t id,
        uint32_t token,
        const std::map<uint32_t, std::unique_ptr<phosphor::dump::Entry>>&
            entries);

    /**
     * @brief Converts a string to uppercase.
     *
     * @param str The string to convert to uppercase.
     * @return A new string where all characters are converted to uppercase.
     */
    inline static std::string toUpper(const std::string& str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        return result;
    }

    /** @brief sdbusplus DBus bus connection. */
    sdbusplus::bus_t& bus;

    /** @brief Base D-Bus path for dump entries. */
    const std::string& baseEntryPath;

    /** @brief Reference to the managing object for dumps.*/
    phosphor::dump::Manager& mgr;
};

} // namespace openpower::dump
