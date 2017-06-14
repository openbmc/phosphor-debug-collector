#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>
#include <experimental/filesystem>

#include "xyz/openbmc_project/Dump/Internal/Create/server.hpp"
#include "dump_entry.hpp"

namespace phosphor
{
namespace dump
{

namespace fs = std::experimental::filesystem;
using Type =
    sdbusplus::xyz::openbmc_project::Dump::Internal::server::Create::Type;

using CreateIface = sdbusplus::server::object::object<
                    sdbusplus::xyz::openbmc_project::Dump::server::Create>;

/** @class Manager
 *  @brief OpenBMC Dump  manager implementation.
 *  @details A concrete implementation for the
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
         *  @param[in] loop - Dump manager sd_event loop.
         *  @param[in] path - Path to attach at.
         */
        Manager(sdbusplus::bus::bus& bus, sd_event* loop, const char* path) :
            CreateIface(bus, path),
            bus(bus),
            eventLoop(loop),
            lastEntryId(0)
        {}

        /** @brief Implementation for CreateDump
         *  Method to create Dump.
         *
         *  @return id[uint32_t] - The Dump entry id number.
         */
        uint32_t createDump() override;

        /** @brief Create Dump entry d-bus object
         *  @param[in] fullPath - Full path of the Dump file name
         */
        void createEntry(const fs::path& fullPath);

        /**  @brief Capture BMC Dump based on the Dump type.
          *  @param[in] type - Type of the Dump.
          *  @param[in] fullPaths - List of absolute paths to the files
          *             to be included as part of Dump package.
          *  @return id[uint32_t] - The Dump entry id number.
          */
        uint32_t captureDump(
            Type type,
            const std::vector<std::string>& fullPaths);

        /** @brief Erase specified entry d-bus object
          *
          * @param[in] entryId - unique identifier of the entry
          */
        void erase(uint32_t entryId);

    private:
        /** @brief sd_event_add_child callback
          *
          *  @param[in] s - event source, floating (unused) in our case
          *  @param[in] si - signal info
          *  @param[in] userdata - pointer to Watch object
          *
          *  @returns 0 on success, -1 on fail
          */
        static int callback(sd_event_source* s,
                            const siginfo_t* si,
                            void* userdata)
        {
            //No specific action required in
            //the sd_event_add_child callback.
            return 0;
        }

        /** @brief sdbusplus DBus bus connection. */
        sdbusplus::bus::bus& bus;

        /** @brief sdbusplus Dump event loop */
        sd_event* eventLoop;

        /** @brief Dump Entry dbus objects map based on entry id */
        std::map<uint32_t, std::unique_ptr<Entry>> entries;

        /** @brief Id of the last Dump entry */
        uint32_t lastEntryId;
};

} // namespace dump
} // namespace phosphor
