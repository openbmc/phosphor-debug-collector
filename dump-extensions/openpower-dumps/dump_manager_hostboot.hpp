#pragma once

#include "dump_manager_local.hpp"
#include "dump_utils.hpp"
#include "watch.hpp"
#include "xyz/openbmc_project/Dump/Internal/Create/server.hpp"

#include <experimental/filesystem>

namespace openpower
{
namespace dump
{
namespace hostboot
{

using UserMap = phosphor::dump::inotify::UserMap;
using Type =
    sdbusplus::xyz::openbmc_project::Dump::Internal::server::Create::Type;
namespace fs = std::experimental::filesystem;

using Watch = phosphor::dump::inotify::Watch;

/** @class Manager
 *  @brief Hostboot Dump  manager implementation.
 *  @details An implementation for managing the
 *  Hostboot dump.
 */
class Manager : public phosphor::dump::local::Manager
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
     *  @param[in] baseEntryPath - Base path for dump entry.
     *  @param[in] filePath - Path where the dumps are stored.
     *  @param[in] dumpInternalMgr - Reference of internal dump manager.
     */
    Manager(sdbusplus::bus::bus& bus, const phosphor::dump::EventPtr& event,
            const char* path, const std::string& baseEntryPath,
            const char* filePath,
            phosphor::dump::internal::Manager& dumpInternalMgr,
            std::string dumpFileRegex) :
        phosphor::dump::local::CreateIface(bus, path),
        phosphor::dump::Iface(bus, path, true), phosphor::dump::local::Manager(
                                                    bus, event, path,
                                                    baseEntryPath, filePath,
                                                    dumpInternalMgr,
                                                    dumpFileRegex)
    {
        // Register the dump types handled by this dump manager.
        dumpInternalMgr.typeMap.emplace(
            Type::Bootfailure, std::move(phosphor::dump::internal::DumpInfo(
                                   "bootfailure", *this)));
    }

    /** @brief Calculate per dump allowed size based on the available
     *        size in the dump location.
     *  @returns dump size in kilobytes.
     */
    virtual size_t getAllowedSize();
};

} // namespace hostboot
} // namespace dump
} // namespace openpower
