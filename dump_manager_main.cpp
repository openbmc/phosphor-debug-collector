#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>
#include "config.h"

int main(int argc, char *argv[])
{
    auto bus = sdbusplus::bus::new_default();

    // Add sdbusplus ObjectManager for the 'root' path of the DUMP manager.
    sdbusplus::server::manager::manager objManager(bus, DUMP_OBJPATH);

    bus.request_name(DUMP_BUSNAME);

    while(true)
    {
        bus.process_discard();
        bus.wait();
    }

    return 0;
}
