#include <sdbusplus/bus.hpp>
#include <phosphor-logging/elog-errors.hpp>

#include "xyz/openbmc_project/Common/error.hpp"
#include "config.h"
#include "dump_manager.hpp"
#include "dump_internal.hpp"

int main(int argc, char* argv[])
{
    using namespace phosphor::logging;
    using InternalFailure =
        sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

    auto bus = sdbusplus::bus::new_default();
    sd_event* event = nullptr;
    auto rc = sd_event_default(&event);
    if (rc < 0)
    {
        log<level::ERR>("Error occurred during the sd_event_default",
                        entry("rc=%d", rc));
        report<InternalFailure>();
        return -1;
    }
    phosphor::dump::EventPtr eventP{event};
    event = nullptr;

    // Add sdbusplus ObjectManager for the 'root' path of the DUMP manager.
    sdbusplus::server::manager::manager objManager(bus, DUMP_OBJPATH);
    bus.request_name(DUMP_BUSNAME);

    try
    {
        phosphor::dump::Manager manager(bus, eventP, DUMP_OBJPATH);
        phosphor::dump::internal::Manager mgr(bus, OBJ_INTERNAL);
        bus.attach_event(eventP.get(), SD_EVENT_PRIORITY_NORMAL);
        auto rc = sd_event_loop(eventP.get());
        if (rc < 0)
        {
            log<level::ERR>("Error occurred during the sd_event_loop",
                            entry("rc=%d", rc));
            elog<InternalFailure>();
        }
    }

    catch (InternalFailure& e)
    {
        commit<InternalFailure>();
        return -1;
    }

    return 0;
}
