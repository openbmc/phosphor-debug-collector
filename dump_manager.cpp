#include "dump_manager.hpp"

#include <fmt/core.h>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>

namespace phosphor
{
namespace dump
{

using namespace phosphor::logging;

void Manager::erase(uint32_t entryId)
{
    entries.erase(entryId);
}

void Manager::deleteAll()
{

    log<level::INFO>("dump_manager deleteAll");

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
