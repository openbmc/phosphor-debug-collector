#include "base_dump_entry.hpp"

#include "dump_manager.hpp"

#include <phosphor-logging/lg2.hpp>
#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{

using namespace phosphor::logging;

void BaseEntry::delete_()
{
    lg2::error(">Base entry delete");
    // Remove Dump entry D-bus object
    parent.erase(id);
    lg2::error("<Base entry delete");
}

} // namespace dump
} // namespace phosphor
