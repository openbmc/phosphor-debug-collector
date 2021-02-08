#pragma once

#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "xyz/openbmc_project/Dump/Internal/Create/server.hpp"

#include <systemd/sd-event.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

namespace phosphor
{
namespace dump
{
namespace internal
{

using CreateIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Dump::Internal::server::Create>;
using Type =
    sdbusplus::xyz::openbmc_project::Dump::Internal::server::Create::Type;

/** @struct DumpInfo
 *  @brief Association between dumpp type and handling manager
 */
struct DumpInfo
{
    std::string type;                 // Type of the dump
    phosphor::dump::Manager& manager; // Handling dump manager

    /** @brief Constructor for the structure.
     *  @param[in] type - Type of the dump
     *  @param[in] manager - Dump handler
     */
    DumpInfo(std::string type, phosphor::dump::Manager& manager) :
        type(type), manager(manager)
    {
    }
};

/** @class Manager
 *  @brief Implementation for the
 *         xyz.openbmc_project.Dump.Internal.Create DBus API.
 */
class Manager : public CreateIface
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
     *  @param[in] dumpMgr - Dump Manager object
     *  @param[in] path - Path to attach at.
     */
    Manager(sdbusplus::bus::bus& bus, const EventPtr& event, const char* path) :
        CreateIface(bus, path), eventLoop(event.get()){};

    /**  @brief Implementation for Create
     *   Create BMC Dump based on the Dump type.
     *
     *   @param[in] type - Type of the Dump.
     *   @param[in] fullPaths - List of absolute paths to the files
     *             to be included as part of Dump package.
     */
    void create(Type type, std::vector<std::string> fullPaths) override;

    /** @brief Capture BMC Dump based on the Dump type.
     *  @param[in] type - Type of the Dump.
     *  @param[in] fullPaths - List of absolute paths to the files
     *             to be included as part of Dump package.
     *  @return id - The Dump entry id number.
     */
    uint32_t captureDump(Type type, const std::vector<std::string>& fullPaths);

    /** @brief map for holding the dump type and associations */
    static std::map<Type, DumpInfo> typeMap;

  private:
    /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

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
        // No specific action required in
        // the sd_event_add_child callback.
        return 0;
    }
};

} // namespace internal
} // namespace dump
} // namespace phosphor
