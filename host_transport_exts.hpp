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
    // Delete the copy constructor and copy assignment operator.
    HostTransport(const HostTransport&) = delete;
    HostTransport& operator=(const HostTransport&) = delete;

    // Provide a static method to get the instance.

    static HostTransport* getInstance()
    {
        static HostTransport* instance = new HostTransport();
        return instance;
    }

    void requestOffload(uint32_t id);
    void requestDelete(uint32_t id, phosphor::dump::DumpTypes dumpType);

  private:
    // Make the constructor private.
    HostTransport() = default;
};
} // namespace host
} // namespace dump
} // namespace phosphor
