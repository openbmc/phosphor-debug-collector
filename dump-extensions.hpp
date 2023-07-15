#include "base_dump_manager.hpp"
#include "dump_manager.hpp"
#include "host_transport_exts.hpp"

#include <memory>
#include <vector>

namespace phosphor
{
namespace dump
{

using DumpManagerList =
    std::vector<std::unique_ptr<phosphor::dump::BaseManager>>;
/**
 * @brief load the dump extensions
 *
 * @param[in] bus - Bus to attach to
 * @param[out] dumpMgrList - list dump manager objects.
 *
 */
void loadExtensions(sdbusplus::bus_t& bus,
                    phosphor::dump::host::HostTransport& hostTransport,
                    DumpManagerList& dumpMgrList);
} // namespace dump
} // namespace phosphor
