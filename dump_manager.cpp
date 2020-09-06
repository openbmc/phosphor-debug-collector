#include "dump_manager.hpp"

namespace phosphor
{
namespace dump
{

void Manager::erase(uint32_t entryId)
{
    entries.erase(entryId);
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
