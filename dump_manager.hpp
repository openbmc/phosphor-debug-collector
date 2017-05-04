#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include "dump_entry.hpp"

namespace phosphor
{
namespace Dump
{

/** @class Manager
 *  @brief OpenBMC dump manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Internal.Manager DBus API.
 */
class Manager //: public ManagerIfaces
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
        /** @brief Constructor for the Log Manager object
         *  @param[in] bus - DBus bus to attach to.
         *  @param[in] obj - Object path to attach to.
         */
        Manager(sdbusplus::bus::bus& bus,
                const char* obj);
};

} // namespace dump
} // namespace phosphor
