#include "dump_entry.hpp"
#include "dump_manager.hpp"

namespace phosphor
{
namespace dump
{

void Entry::delete_()
{
    //Delete Dump file from flash
    if (fs::exists(dumpFile))
    {
       fs::remove(dumpFile);
    }
    // Remove Dump entry D-bus object
    parent.erase(id);   
}


} // namespace dump
} // namepsace phosphor
