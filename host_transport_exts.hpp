#pragma once

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
    static HostTransport* getInstance(uint32_t dumpType)
    {
        static std::unordered_map<uint32_t, std::unique_ptr<HostTransport>>
            instances;

        // Create a new instance if it doesn't exist, or return existing one.
        auto it = instances.find(dumpType);
        if (it == instances.end())
        {
            // Create a new instance and insert it into the map.
            std::unique_ptr<HostTransport> newInstance(
                new HostTransport(dumpType));
            auto result = instances.insert(
                std::make_pair(dumpType, std::move(newInstance)));

            return result.first->second.get();
        }
        else
        {
            return it->second.get();
        }
    }

    void requestOffload(uint32_t id);
    void requestDelete(uint32_t id);

  private:
    // Make the constructor private.
    HostTransport(uint32_t type) : dumpType(type) {}

    uint32_t dumpType;
};
} // namespace host
} // namespace dump
} // namespace phosphor
