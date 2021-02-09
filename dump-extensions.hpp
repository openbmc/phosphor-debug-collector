#include "dump_internal.hpp"
#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "file_watch_manager.hpp"

#include <memory>
#include <vector>

namespace phosphor
{
namespace dump
{

using DumpManagerList = std::vector<std::unique_ptr<phosphor::dump::Manager>>;
using FileWatchManagerList =
    std::vector<std::unique_ptr<phosphor::dump::filewatch::Manager>>;
/**
 * @brief load the dump extensions
 *
 * @param[in] bus - Bus to attach to
 * @param[in] event - Dump manager sd_event loop.
 * @param[in] dumpInternalMgr - Reference of internal dump manager.
 * @param[out] dumpMgrList - list dump manager objects.
 *
 */
void loadExtensions(sdbusplus::bus::bus& bus, const EventPtr& event,
                    phosphor::dump::internal::Manager& dumpInternalMgr,
                    DumpManagerList& dumpMgrList);
/**
 * @brief load file watch extensions
 *
 * @param[in] event -
 * @param[out] fileWatchList - List of file watch objects
 *
 */
void loadFileWatchList(const EventPtr& event,
                       FileWatchManagerList& fileWatchList);
} // namespace dump
} // namespace phosphor
