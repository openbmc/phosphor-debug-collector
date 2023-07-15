#pragma once

#include "dump_types.hpp"

#include <stdint.h>

#include <memory>
#include <unordered_map>

namespace phosphor
{
namespace dump
{
namespace host
{
class HostTransport
{
  public:
    HostTransport() = default;

    // Provide a static method to get the instance.
    void requestOffload(uint32_t id);
    void requestDelete(uint32_t id, phosphor::dump::DumpTypes dumpType);
};
} // namespace host
} // namespace dump
} // namespace phosphor
