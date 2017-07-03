#include <unistd.h>
#include <sys/inotify.h>

#include "core_manager.hpp"

namespace phosphor
{
namespace dump
{
namespace core
{
namespace manager
{

void watchCallback(UserMap fileInfo)
{
    for (auto& i : fileInfo)
    {
        // For any new dump file create dump entry object.
        if (IN_CLOSE_WRITE == i.second)
        {
            //TODO openbmc/openbmc#1795 Enable Dump collection function here
        }
    }
}

} // namespace manager
} // namespace core
} // namespace dump
} // namespace phosphor
