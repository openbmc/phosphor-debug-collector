#include "config.h"

#include "ramoops_manager.hpp"
#include "watch.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <filesystem>
#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/bus.hpp>

int main()
{
    using namespace phosphor::logging;
    using InternalFailure =
        sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

    namespace fs = std::filesystem;

    auto bus = sdbusplus::bus::new_default();
    sd_event* event = nullptr;
    auto rc = sd_event_default(&event);
    if (rc < 0)
    {
        log<level::ERR>("Error occurred during the sd_event_default",
                        entry("RC=%d", rc));
        report<InternalFailure>();
        return EXIT_FAILURE;
    }
    phosphor::dump::EventPtr eventP{event};
    event = nullptr;

    fs::path filePath(SYSTEMD_PSTORE_PATH);
    if (!fs::exists(filePath))
    {
        log<level::ERR>("Pstore file path is not exists",
                        entry("FILE_PATH = %s", SYSTEMD_PSTORE_PATH));
        return EXIT_FAILURE;
    }

    try
    {
        phosphor::dump::ramoops::Manager manager(eventP, SYSTEMD_PSTORE_PATH);

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
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
