#include "system_dump_entry.hpp"

#include "dump_utils.hpp"
#include "host_transport_exts.hpp"

namespace phosphor
{
namespace dump
{
namespace system
{

void Entry::initiateOffload(std::string uri)
{
    phosphor::dump::Entry::initiateOffload(uri);
    phosphor::dump::host::requestOffload(sourceDumpId());
}

void Entry::delete_()
{
    auto srcDumpID = sourceDumpId();

    // Remove Dump entry D-bus object
    phosphor::dump::Entry::delete_();

    // Remove host system dump when host is up by using source dump id
    // which is present in system dump entry dbus object as a property.
    BootProgress bootProgressStatus = phosphor::dump::getBootProgress();
    if ((bootProgressStatus == BootProgress::SystemInitComplete) ||
        (bootProgressStatus == BootProgress::OSStart) ||
        (bootProgressStatus == BootProgress::OSRunning))
    {
        phosphor::dump::host::requestDelete(srcDumpID);
    }
}
} // namespace system
} // namespace dump
} // namespace phosphor
