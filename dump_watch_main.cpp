#include <cstdlib>
#include <exception>
#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
#include "config.h"
#include "dump_watch.hpp"

int main(int argc, char* argv[])
{
    auto bus = sdbusplus::bus::new_default();
    sd_event* loop = nullptr;
    sd_event_default(&loop);

    try
    {
        phosphor::dump::Watch watch(loop);
        bus.attach_event(loop, SD_EVENT_PRIORITY_NORMAL);
        sd_event_loop(loop);
    }
    catch (std::exception& e)
    {
        using namespace phosphor::logging;
        log<level::ERR>(e.what());
        return -1;
    }

    sd_event_unref(loop);

    return 0;
}
