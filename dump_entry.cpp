#include <phosphor-logging/log.hpp>

#include "dump_entry.hpp"
#include "dump_manager.hpp"

namespace phosphor
{
namespace dump
{

using namespace phosphor::logging;

void Entry::delete_()
{
    //Delete Dump file from Permanent location
    try
    {
        fs::remove(file);
    }
    catch (fs::filesystem_error& e)
    {
        //Log Error message and continue
        log<level::ERR>(e.what());
    }

    // Remove Dump entry D-bus object
    parent.erase(id);
}

} // namespace dump
} // namepsace phosphor
