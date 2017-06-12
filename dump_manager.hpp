#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include "xyz/openbmc_project/Dump/Create/server.hpp"
#include "dump_entry.hpp"

namespace phosphor
{
namespace dump
{

using createIface = sdbusplus::server::object::object<
                    sdbusplus::xyz::openbmc_project::Dump::server::Create>;

/** @class Manager
 *  @brief OpenBMC Dump  manager implementation.
 *  @details A concrete implementation Implementation for the
 *  xyz.openbmc_project.Dump.Create DBus API.
 */
class Manager : public createIface
{
    public:
        Manager() = delete;
        Manager(const Manager&) = delete;
        Manager& operator=(const Manager&) = delete;
        Manager(Manager&&) = delete;
        Manager& operator=(Manager&&) = delete;
        virtual ~Manager() = default;

        /** @brief Constructor to put object onto bus at a dbus path.
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         */
        Manager(sdbusplus::bus::bus& bus, const char* path) :
            createIface(bus, path),
            busLog(bus),
            entryId(0) {};

        /** @brief Implementation for CreateDump
         *  Method to create BMC Dump.
         *
         *  @return id[uint32_t] - The Dump entry id number.
         */
        uint32_t createDump() override;

    private:
        /** @brief Persistent sdbusplus DBus bus connection. */
        sdbusplus::bus::bus& busLog;

        /** @brief Persistent map of Dump Entry dbus objects and their ID */
        std::map<uint32_t, std::unique_ptr<Entry>> entries;

        /** @brief Id of last Dump entry */
        uint32_t entryId;
};

} // namespace dump
} // namespace phosphor
