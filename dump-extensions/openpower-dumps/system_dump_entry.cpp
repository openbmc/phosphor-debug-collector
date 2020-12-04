#include "system_dump_entry.hpp"

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
    // Remove host system dump by using source dump id which is present in
    // system dump entry dbus object as a property.
    phosphor::dump::host::requestDelete(sourceDumpId());

    // Remove Dump entry D-bus object
    phosphor::dump::Entry::delete_();
}
} // namespace system
} // namespace dump
} // namespace phosphor
