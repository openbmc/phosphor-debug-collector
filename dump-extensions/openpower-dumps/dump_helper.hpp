#pragma once

#include "dump_utils.hpp"
#include "host_transport_exts.hpp"

namespace openpower
{
namespace dump
{

class Manager;

class DumpHelper
{
  public:
    DumpHelper() = delete;
    DumpHelper(const DumpHelper&) = default;
    DumpHelper& operator=(const DumpHelper&) = delete;
    DumpHelper(DumpHelper&&) = delete;
    DumpHelper& operator=(DumpHelper&&) = delete;
    virtual ~DumpHelper() = default;

    DumpHelper(sdbusplus::bus_t& bus, openpower::dump::Manager& mgr) :
        bus(bus), mgr(mgr)
    {}

    virtual std::string createDump(phosphor::dump::DumpCreateParams& params) = 0;

    virtual std::string
        createEntry(uint32_t dumpId, uint64_t size,
                    phosphor::dump::OperationStatus status,
                    const std::string& originatorId,
                    phosphor::dump::OriginatorTypes originatorType,
                    phosphor::dump::host::HostTransport& hostTransport) = 0;

  protected:
    sdbusplus::bus_t& bus;
    openpower::dump::Manager& mgr;
};
} // namespace dump
} // namespace openpower
