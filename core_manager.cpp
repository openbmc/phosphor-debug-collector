#include <unistd.h>
#include <sys/inotify.h>

#include "core_manager.hpp"

namespace phosphor
{
namespace dump
{
namespace core
{
void Manager::watchCallback(std::map<fs::path, uint32_t> fileInfo)
{
    if (fileInfo.empty())
    {
        return;
    }

    for (auto& i : fileInfo)
    {
        // For any new dump file create dump entry object.
        if (IN_CLOSE_WRITE == i.second)
        {
            //TODO openbmc/openbmc#1710 Enable Dump collection function here
        }
    }
}

} // namespace core
} // namespace dump
} // namespace phosphor
