#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include "xyz/openbmc_project/Dump/Internal/Create/server.hpp"

namespace phosphor
{
namespace dump
{
namespace internal
{

using createIface = sdbusplus::server::object::object<
                    sdbusplus::xyz::openbmc_project::Dump::Internal::server::Create>;

/** @class Manager
 *  @brief Contains BMC Dump Internal Create dbus object.
 *  @details Implementation for the
 *  xyz.openbmc_project.Dump.Internal.Create DBus API.
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
            createIface(bus, path) {};

        /**  @brief Implementation for Create
          *  Create BMC Dump based on the Dump type.
          *
          *  @param[in] type - Type of the Dump.
          *  @param[in] fullPaths - List of absolute paths to the files
          *             to be included as part of Dump package.
          */
        void create(
            Type type,
            std::vector<std::string> fullPaths) override;

};

} // namespace internal
} // namespace dump
} // namespace phosphor
