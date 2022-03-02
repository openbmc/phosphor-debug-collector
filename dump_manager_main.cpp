#include "config.h"

#include "dump-extensions.hpp"
#include "dump_internal.hpp"
#include "dump_manager.hpp"
#include "dump_manager_bmc.hpp"
#include "dump_manager_faultlog.hpp"
#include "elog_watch.hpp"
#include "watch.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <fmt/core.h>

#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/bus.hpp>

#include <memory>
#include <vector>

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
        log<level::ERR>(
            fmt::format("Error occurred during the sd_event_default, rc({})",
                        rc)
                .c_str());
        report<InternalFailure>();
        return rc;
    }
    // dump_utils.hpp
    // using EventPtr = std::unique_ptr<sd_event, EventDeleter>;
    phosphor::dump::EventPtr eventP{event};
    event = nullptr;

    // Blocking SIGCHLD is needed for calling sd_event_add_child
    sigset_t mask;
    if (sigemptyset(&mask) < 0)
    {
        log<level::ERR>(
            fmt::format("Unable to initialize signal set, errno({})", errno)
                .c_str());
        return EXIT_FAILURE;
    }

    if (sigaddset(&mask, SIGCHLD) < 0)
    {
        log<level::ERR>(
            fmt::format("Unable to add signal to signal set, errno({})", errno)
                .c_str());
        return EXIT_FAILURE;
    }

    // Block SIGCHLD first, so that the event loop can handle it
    if (sigprocmask(SIG_BLOCK, &mask, nullptr) < 0)
    {
        log<level::ERR>(
            fmt::format("Unable to block signal, errno({})", errno).c_str());
        return EXIT_FAILURE;
    }

    // Add sdbusplus ObjectManager for the 'root' path of the DUMP manager.
    sdbusplus::server::manager::manager objManager(bus, DUMP_OBJPATH);
    bus.request_name(DUMP_BUSNAME);

    try
    {
        // dump-extensions.hpp
        // using DumpManagerList =
        // std::vector<std::unique_ptr<phosphor::dump::Manager>>;
        phosphor::dump::DumpManagerList dumpMgrList{};
        std::unique_ptr<phosphor::dump::bmc::Manager> bmcDumpMgr =
            std::make_unique<phosphor::dump::bmc::Manager>(
                bus, eventP, BMC_DUMP_OBJPATH, BMC_DUMP_OBJ_ENTRY,
                BMC_DUMP_PATH);

        // dump_internal.hpp
        phosphor::dump::bmc::internal::Manager mgr(bus, *bmcDumpMgr,
                                                   OBJ_INTERNAL);

        dumpMgrList.push_back(std::move(bmcDumpMgr));

        // dumpMgrList.push_back(std::make_unique<phosphor::dump::faultlog::Manager>(
        // bus, FAULTLOG_DUMP_OBJPATH, FAULTLOG_DUMP_OBJ_ENTRY));

        std::unique_ptr<phosphor::dump::faultlog::Manager> faultLogMgr =
            std::make_unique<phosphor::dump::faultlog::Manager>(
                bus, eventP, FAULTLOG_DUMP_OBJPATH, FAULTLOG_DUMP_OBJ_ENTRY,
                FAULTLOG_DUMP_PATH);

        dumpMgrList.push_back(std::move(faultLogMgr));

        phosphor::dump::loadExtensions(bus, dumpMgrList);

        // Restore dbus objects of all dumps
        for (auto& dmpMgr : dumpMgrList)
        {
            dmpMgr->restore();
        }

        phosphor::dump::elog::Watch eWatch(bus, mgr);
        bus.attach_event(eventP.get(), SD_EVENT_PRIORITY_NORMAL);

        auto rc = sd_event_loop(eventP.get());
        if (rc < 0)
        {
            log<level::ERR>(
                fmt::format("Error occurred during the sd_event_loop, rc({})",
                            rc)
                    .c_str());
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
