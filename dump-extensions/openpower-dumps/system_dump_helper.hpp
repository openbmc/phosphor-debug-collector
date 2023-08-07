#pragma once

#include "base_dump_entry.hpp"
#include "base_dump_manager.hpp"
#include "dump_helper.hpp"
#include "host_transport_exts.hpp"
#include "new_dump_entry.hpp"
#include "system_dump_entry.hpp"

namespace openpower
{
namespace dump
{

class Manager;
namespace system
{

class DumpHelper : public openpower::dump::DumpHelper
{
  public:
    DumpHelper() = delete;
    DumpHelper(const DumpHelper&) = default;
    DumpHelper& operator=(const DumpHelper&) = delete;
    DumpHelper(DumpHelper&&) = delete;
    DumpHelper& operator=(DumpHelper&&) = delete;
    virtual ~DumpHelper() = default;

    DumpHelper(sdbusplus::bus_t& bus, openpower::dump::Manager& mgr,
               phosphor::dump::host::HostTransport& hostTransport) :
        openpower::dump::DumpHelper(bus, mgr),
        hostTransport(hostTransport)
    {}

    std::string createDump(phosphor::dump::DumpCreateParams& params) override;

    std::string createEntry(
        uint32_t dumpId, uint64_t size, phosphor::dump::OperationStatus status,
        const std::string& originatorId,
        phosphor::dump::OriginatorTypes originatorType,
        phosphor::dump::host::HostTransport& hostTransport) override;

    static constexpr auto dumpType = phosphor::dump::DumpTypes::SYSTEM;

  private:
    bool isHostStateValid();

    phosphor::dump::host::HostTransport& hostTransport;
};
} // namespace system
} // namespace dump
} // namespace openpower
