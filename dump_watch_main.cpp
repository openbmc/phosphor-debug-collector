#include "dump_watch.hpp"
#include "elog-errors.hpp"
#include "xyz/openbmc_project/Dump/Monitor/error.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>

int main(int argc, char* argv[])
{
    sd_event* loop = nullptr;
    sd_event_default(&loop);

    using namespace phosphor::logging;

    using InvalidCorePath =
        sdbusplus::xyz::openbmc_project::Dump::Monitor::Error::InvalidCorePath;
    using InternalFailure =
        sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

    try
    {
        phosphor::dump::inotify::Watch watch(loop);
        sd_event_loop(loop);
    }

    catch (InvalidCorePath& e)
    {
        commit<InvalidCorePath>();
        return -1;
    }

    catch (InternalFailure& e)
    {
        commit<InternalFailure>();
        return -1;
    }

    sd_event_unref(loop);

    return 0;
}
