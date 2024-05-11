#include "config.h"

#include "dump-extensions.hpp"

#include "dump_manager_openpower.hpp"

#include <xyz/openbmc_project/Dump/Create/client.hpp>

namespace phosphor
{
namespace dump
{

void loadExtensions(sdbusplus::bus_t& bus, DumpManagerList& dumpList)
{
    using DumpCreate = sdbusplus::client::xyz::openbmc_project::dump::Create<>;
    auto opDumpPath =
        sdbusplus::message::object_path(DumpCreate::namespace_path::value) /
        DumpCreate::namespace_path::system;
    auto opDumpEntryPath = opDumpPath / "Entry";

    dumpList.push_back(std::make_unique<openpower::dump::Manager>(
        bus, opDumpPath.str.c_str(), opDumpEntryPath));
}
} // namespace dump
} // namespace phosphor
