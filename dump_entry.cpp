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
        fs::remove_all(file.parent_path());
    }
    catch (fs::filesystem_error& e)
    {
        log<level::INFO>("Dump file doesn't exist.",
                         entry("Name=%s", file.c_str()));
    }

    // Remove Dump entry D-bus object
    parent.erase(id);
}

} // namespace dump
} // namepsace phosphor
