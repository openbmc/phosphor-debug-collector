#include "dump_entry.hpp"

#include "dump_manager.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace dump
{

void Entry::delete_()
{
    // Remove Dump entry D-bus object
    parent.erase(id);
}

sdbusplus::message::unix_fd Entry::getFileHandle()
{
    using namespace phosphor::logging;
    lg2::info("getFileHandle is not implemented");
    elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
}

} // namespace dump
} // namespace phosphor
