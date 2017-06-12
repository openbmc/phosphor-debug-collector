#include <sdbusplus/vtable.hpp>
#include "dump_manager.hpp"
#include "dump_create.hpp"
#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{
namespace internal
{

void Manager::create(
    Type type,
    std::vector<std::string> fullPaths)
{
    //TODO Add Dump manager common implementaion function
}

} //namepsace internal

uint32_t Manager::createDump()
{
    //TODO Add appropriate manager common implementaion function
    //TODO return dumpID
    return entryId;
}

} //namespace dump
} //namespace phosphor
