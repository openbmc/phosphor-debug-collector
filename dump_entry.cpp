#include "dump_entry.hpp"

namespace phosphor
{
namespace dump
{

Entry::Entry(sdbusplus::bus::bus& bus, const char* obj): EntryIfaces(bus, obj)
{
}


void Entry::delete_()
{
}

} // namespace dump
} // namepsace phosphor
