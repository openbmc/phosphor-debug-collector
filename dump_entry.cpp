#include "dump_entry.hpp"
#include "dump_manager.hpp"

#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{

using namespace phosphor::logging;

void Entry::delete_()
{
    //Delete Dump file from Permanent location
    if (!fs::remove(file))
    {
        log<level::INFO>("Dump file doesn't exist.",
                         entry("Name=%s", file.c_str()));
    }
    // Remove Dump entry D-bus object
    parent.erase(id);
}

} // namespace dump
} // namepsace phosphor
