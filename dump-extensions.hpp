#include "dump_manager.hpp"

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
 * @param[out] dumpMgrList - list dump manager objects.
 *
 */
void loadExtensions(sdbusplus::bus::bus& bus,
                    const phosphor::dump::EventPtr& event,
                    DumpManagerList& dumpMgrList);
} // namespace dump
} // namespace phosphor
