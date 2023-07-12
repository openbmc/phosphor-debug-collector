#include "dump_manager.hpp"

#include <phosphor-logging/lg2.hpp>

namespace phosphor
{
namespace dump
{

void Manager::erase(uint32_t entryId)
{
    lg2::error("> dump manager erase");
    entries.erase(entryId);
    lg2::error("< dump manager erase");
}

void Manager::deleteAll()
{
    auto iter = entries.begin();
    while (iter != entries.end())
    {
        auto& entry = iter->second;
        ++iter;
        entry->delete_();
    }
}

} // namespace dump
} // namespace phosphor
