#include "faultlog_dump_entry.hpp"

#include <phosphor-logging/lg2.hpp>

namespace phosphor
{
namespace dump
{
namespace faultlog
{

void Entry::delete_()
{
    lg2::info("In faultlog_dump_entry.cpp delete_()");

    // Delete Dump file from Permanent location
    try
    {
        std::filesystem::remove(file);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        // Log Error message and continue
        lg2::error("Failed to delete dump file, errormsg: {ERROR}", "ERROR", e);
    }

    // Remove Dump entry D-bus object
    phosphor::dump::Entry::delete_();
}

} // namespace faultlog
} // namespace dump
} // namespace phosphor
