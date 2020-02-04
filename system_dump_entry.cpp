#include "system_dump_entry.hpp"

#include "pldm_interface.hpp"

namespace phosphor
{
namespace dump
{
namespace system
{
using namespace phosphor::dump::pldm;

void Entry::initiateOffload()
{
    requestOffload(sourceDumpId());
}

} // namespace system
} // namespace dump
} // namespace phosphor
