#include "resource_dump_entry.hpp"

#include "offload-extensions.hpp"

namespace phosphor
{
namespace dump
{
namespace resource
{

void Entry::initiateOffload(std::string uri)
{
    phosphor::dump::Entry::initiateOffload(uri);
    phosphor::dump::host::requestOffload(sourceDumpId());
}

} // namespace resource
} // namespace dump
} // namespace phosphor
