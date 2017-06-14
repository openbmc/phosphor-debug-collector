#include <phosphor-logging/elog-errors.hpp>
#include "xyz/openbmc_project/Common/error.hpp"
#include "config.h"
#include "dump_manager.hpp"
#include "dump_internal.hpp"
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>
#include "config.h"
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include "elog-errors.hpp"
#include <xyz/openbmc_project/Dump/Monitor/error.hpp>
#include "xyz/openbmc_project/Common/error.hpp"
#include "dump_manager.hpp"
#include "dump_entry.hpp"
#include "watch.hpp"

int main(int argc, char* argv[])
{
    auto bus = sdbusplus::bus::new_default();
    using namespace phosphor::logging;
    using InternalFailure =
        sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

    sd_event* loop = nullptr;
    sd_event_default(&loop);

    // Add sdbusplus ObjectManager for the 'root' path of the DUMP manager.
    sdbusplus::server::manager::manager objManager(bus, DUMP_OBJPATH);
    bus.request_name(DUMP_BUSNAME);

    try
    {
        phosphor::dump::Manager manager(bus, loop, DUMP_OBJPATH);
        phosphor::dump::internal::Manager mgr(bus, OBJ_INTERNAL);
        phosphor::dump::report::Watch watch(loop, &manager);
        bus.attach_event(loop, SD_EVENT_PRIORITY_NORMAL);
        sd_event_loop(loop);
    }

    catch (InternalFailure& e)
    {
        commit<InternalFailure>();
        return -1;
    }
    
    sd_event_unref(loop);

    return 0;
}
