#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>
#include "config.h"
#include "dump_manager.hpp"
#include "dump_create.hpp"

int main(int argc, char *argv[])
{
    auto bus = sdbusplus::bus::new_default();

    // Add sdbusplus ObjectManager for the 'root' path of the DUMP manager.
    sdbusplus::server::manager::manager objManager(bus, DUMP_OBJPATH);

    phosphor::dump::Manager manager(bus, DUMP_OBJPATH);

    phosphor::dump::internal::Manager i_manager(bus, OBJ_INTERNAL);

    bus.request_name(DUMP_BUSNAME);

    while(true)
    {
        bus.process_discard();
        bus.wait();
    }

    return 0;
}
