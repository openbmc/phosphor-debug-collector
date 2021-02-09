#include "config.h"

#include "dump-extensions.hpp"
#include "dump_utils.hpp"
#include "hostboot_dump_watch.hpp"

namespace phosphor
{
namespace dump
{

void loadFileWatchList(const EventPtr& event,
                       FileWatchManagerList& fileWatchList)
{
    fileWatchList.push_back(
        std::make_unique<openpower::dump::hostboot_watch::Manager>(event));
}
} // namespace dump
} // namespace phosphor
