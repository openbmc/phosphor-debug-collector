#include "config.h"

#include "core_manager.hpp"
#include "watch.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

int main()
{
    using namespace phosphor::logging;
    using InternalFailure =
        sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

    auto bus = sdbusplus::bus::new_default();
    sd_event* event = nullptr;
    auto rc = sd_event_default(&event);
    if (rc < 0)
    {
        lg2::error("Error occurred during the sd_event_default, rc: {RC}", "RC",
                   rc);
        report<InternalFailure>();
        return -1;
    }
    phosphor::dump::EventPtr eventP{event};
    event = nullptr;

    try
    {
        phosphor::dump::core::Manager manager(eventP);

        auto rc = sd_event_loop(eventP.get());
        if (rc < 0)
        {
            lg2::error("Error occurred during the sd_event_loop, rc: {RC}",
                       "RC", rc);
            elog<InternalFailure>();
        }
    }

    catch (const InternalFailure& e)
    {
        commit<InternalFailure>();
        return -1;
    }

    return 0;
}
