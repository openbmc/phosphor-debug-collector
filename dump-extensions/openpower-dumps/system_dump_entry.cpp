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

} // namespace system
} // namespace dump
} // namespace phosphor
