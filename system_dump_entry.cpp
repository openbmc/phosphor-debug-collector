#include "system_dump_entry.hpp"

#include "pldm_interface.hpp"

namespace phosphor
{
namespace dump
{
namespace system
{
using namespace phosphor::dump::pldm;

void Entry::initiateOffload(std::string uri)
{
    phosphor::dump::Entry::initiateOffload(uri);
    requestOffload(sourceDumpId());
}

} // namespace system
} // namespace dump
} // namespace phosphor
