#include "dump-extensions.hpp"

namespace phosphor
{
namespace dump
{
void loadExtensions(sdbusplus::bus::bus&, const EventPtr&,
                    phosphor::dump::internal::Manager&, DumpManagerList&)
{
}

void loadFileWatchList(const EventPtr&, FileWatchManagerList&)
{
}
} // namespace dump
} // namespace phosphor
