#include "config.h"

#include "core_manager.hpp"
#include "dump-extensions-watch.hpp"
#include "watch.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
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
        log<level::ERR>("Error occurred during the sd_event_default",
                        entry("RC=%d", rc));
        report<InternalFailure>();
        return -1;
    }
    phosphor::dump::EventPtr eventP{event};
    event = nullptr;

    phosphor::dump::FileWatchManagerList fileWatchManagerList = {};

    try
    {
        std::unique_ptr<phosphor::dump::core::Manager> manager =
            std::make_unique<phosphor::dump::core::Manager>(eventP);
        fileWatchManagerList.push_back(std::move(manager));
        loadFileWatchList(eventP, fileWatchManagerList);

        auto rc = sd_event_loop(eventP.get());
        if (rc < 0)
        {
            log<level::ERR>("Error occurred during the sd_event_loop",
                            entry("RC=%d", rc));
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
