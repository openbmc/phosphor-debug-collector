#include "dump_internal.hpp"
#include "dump_manager.hpp"
#include "dump_utils.hpp"

#include <memory>
#include <vector>

namespace phosphor
{
namespace dump
{

using DumpManagerList = std::vector<std::unique_ptr<phosphor::dump::Manager>>;
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
} // namespace dump
} // namespace phosphor
