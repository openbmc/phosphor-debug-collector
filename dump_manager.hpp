#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>
#include "xyz/openbmc_project/Dump/Internal/Create/server.hpp"
#include "dump_entry.hpp"
#include <experimental/filesystem>

namespace phosphor
{
namespace dump
{

namespace fs = std::experimental::filesystem;
using DumpType = 
         sdbusplus::xyz::openbmc_project::Dump::Internal::server::Create::Type;

using CreateIface = sdbusplus::server::object::object<
         sdbusplus::xyz::openbmc_project::Dump::server::Create>;

/** @class Manager
 *  @brief OpenBMC Dump  manager implementation.
 *  @details A concrete implementation Implementation for the
 *  xyz.openbmc_project.Dump.Create DBus API.
 */
class Manager : public CreateIface
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
         */
        Manager(sdbusplus::bus::bus& bus, const char* path) :
            CreateIface(bus, path),
            busLog(bus),
            entryId(0) {};

        /** @brief Implementation for CreateDump
         *  Method to create BMC Dump.
         *
         *  @return id[uint32_t] - The Dump entry id number.
         */
        uint32_t createDump() override;
   
        /** @brief Create BMC Dump entry d-bus object
         *  @param[in] fullPath - List of absolute paths to the required 
         *             to create d-bus entry object
         */
        void createEntry(fs::path fullPath); 

        /**  @brief Capture BMC Dump based on the Dump type.
          *  @param[in] type - Type of the Dump.
          *  @param[in] fullPaths - List of absolute paths to the files
          *             to be included as part of Dump package.
          *  @return id[uint32_t] - The Dump entry id number.
          */
        uint32_t captureDump(
            DumpType type,
            std::vector<std::string> fullPaths);

        /** @brief Erase specified entry d-bus object
          *
          * @param[in] entryId - unique identifier of the entry
          */
        void erase(uint32_t entryId);

    private:
 
        /** @brief Persistent sdbusplus DBus bus connection. */
        sdbusplus::bus::bus& busLog;

        /** @brief Persistent map of Dump Entry dbus objects,
                   file name and their ID
         */
        std::map<uint32_t, std::unique_ptr<Entry>> entries;

        /** @brief Id of the last Dump entry */
        uint32_t entryId;
};

} // namespace dump
} // namespace phosphor
