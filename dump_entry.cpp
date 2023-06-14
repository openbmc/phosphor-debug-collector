#include "dump_entry.hpp"

#include "dump_manager.hpp"

namespace phosphor
{
namespace dump
{

void Entry::delete_()
{
    // Remove Dump entry D-bus object
    parent.erase(id);
}

} // namespace dump
} // namespace phosphor
