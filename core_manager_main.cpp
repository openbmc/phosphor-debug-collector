#include <sdbusplus/bus.hpp>
#include <phosphor-logging/elog-errors.hpp>

#include "xyz/openbmc_project/Common/error.hpp"
#include "config.h"
#include "core_manager.hpp"
#include "watch.hpp"

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

    try
    {
        phosphor::dump::inotify::Watch watch(
            eventP,
            IN_NONBLOCK,
            IN_CLOSE_WRITE,
            EPOLLIN,
            CORE_FILE_DIR,
            std::bind(
                &phosphor::dump::core::Manager::watchCallback,
                std::placeholders::_1));

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
