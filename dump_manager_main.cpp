#include "config.h"
#include "dump_manager.hpp"
#include "dump_internal.hpp"
#include "watch.hpp"
#include <sdbusplus/bus.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include "xyz/openbmc_project/Common/error.hpp"
#include <xyz/openbmc_project/Dump/Manager/error.hpp>

int main(int argc, char* argv[])
{
    using namespace phosphor::logging;
    using InternalFailure =
        sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
    using InvalidDumpPath =
        sdbusplus::xyz::openbmc_project::Dump::Manager::Error::InvalidDumpPath;

    auto bus = sdbusplus::bus::new_default();
    sd_event* loop = nullptr;
    sd_event_default(&loop);

    // Add sdbusplus ObjectManager for the 'root' path of the DUMP manager.
    sdbusplus::server::manager::manager objManager(bus, DUMP_OBJPATH);
    bus.request_name(DUMP_BUSNAME);

    try
    {
        phosphor::dump::Manager manager(bus, loop, DUMP_OBJPATH);
        phosphor::dump::internal::Manager mgr(bus, OBJ_INTERNAL);
        phosphor::dump::notify::Watch watch(loop,
                             std::bind(
                                  std::mem_fn(
                                     &phosphor::dump::Manager::createEntry),
                                  &manager, std::placeholders::_1));
        bus.attach_event(loop, SD_EVENT_PRIORITY_NORMAL);
        sd_event_loop(loop);
    }

    catch (InternalFailure& e)
    {
        commit<InternalFailure>();
        sd_event_unref(loop);
        return -1;
    }

    catch (InvalidDumpPath& e)
    {
        commit<InvalidDumpPath>();
        sd_event_unref(loop);
        return -1;
    }

    sd_event_unref(loop);

    return 0;
}
