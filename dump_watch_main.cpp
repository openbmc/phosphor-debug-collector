#include <exception>
#include "dump_watch.hpp"
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include "elog-errors.hpp"
#include <xyz/openbmc_project/Dump/Monitor/error.hpp>

int main(int argc, char* argv[])
{
    sd_event* loop = nullptr;
    sd_event_default(&loop);

    using namespace phosphor::logging;

    using InvCorePath =
        sdbusplus::xyz::openbmc_project::Dump::Monitor::Error::InvalidCorePath;
    using IntFailure =
        sdbusplus::xyz::openbmc_project::Dump::Monitor::Error::InternalFailure;

    try
    {
        phosphor::dump::Watch watch(loop);
        sd_event_loop(loop);
    }

    catch (InvCorePath& e)
    {
        commit<InvCorePath>();
        return -1;
    }

    catch (IntFailure& e)
    {
        commit<IntFailure>();
        return -1;
    }

    sd_event_unref(loop);

    return 0;
}
