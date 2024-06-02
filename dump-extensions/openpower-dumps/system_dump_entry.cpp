#include "system_dump_entry.hpp"

#include "host_dump_entry.tpp"

namespace openpower::dump::host::system
{

Entry::Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
             uint64_t timeStamp, uint64_t dumpSize, const uint32_t sourceId,
             phosphor::dump::OperationStatus status, std::string originatorId,
             phosphor::dump::originatorTypes originatorType,
             phosphor::dump::Manager& parent) :
    phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, dumpSize,
                          std::string(), status, originatorId, originatorType,
                          parent),
    openpower::dump::host::Entry<Entry>(
        bus, objPath, dumpId, timeStamp, dumpSize, status, originatorId,
        originatorType, parent, TRANSPORT_DUMP_TYPE_IDENTIFIER),
    EntryIfaces(bus, objPath.c_str(), EntryIfaces::action::defer_emit)
{
    sourceDumpId(sourceId);
    if (status == phosphor::dump::OperationStatus::Completed)
    {
        serialize();
    }
    // Emit deferred signal.
    this->openpower::dump::host::system::EntryIfaces::emit_object_added();
}

Entry::Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
             uint64_t timeStamp, uint64_t dumpSize, const uint32_t sourceId,
             phosphor::dump::OperationStatus status, std::string originatorId,
             phosphor::dump::originatorTypes originatorType,
             SystemImpact sysImpact, std::string usrChallenge,
             phosphor::dump::Manager& parent) :
    phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, dumpSize,
                          std::string(), status, originatorId, originatorType,
                          parent),
    openpower::dump::host::Entry<Entry>(
        bus, objPath, dumpId, timeStamp, dumpSize, status, originatorId,
        originatorType, parent, TRANSPORT_DUMP_TYPE_IDENTIFIER),
    EntryIfaces(bus, objPath.c_str(), EntryIfaces::action::defer_emit)
{
    sourceDumpId(sourceId);
    userChallenge(usrChallenge);
    systemImpact(sysImpact);
    // Emit deferred signal.
    this->openpower::dump::host::system::EntryIfaces::emit_object_added();
}

Entry::Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
             phosphor::dump::Manager& parent) :
    phosphor::dump::Entry(bus, objPath.c_str(), dumpId, 0, 0, "",
                          phosphor::dump::OperationStatus::InProgress, "",
                          phosphor::dump::originatorTypes::Internal, parent),
    openpower::dump::host::Entry<Entry>(
        bus, objPath, dumpId, 0, 0, phosphor::dump::OperationStatus::InProgress,
        "", phosphor::dump::originatorTypes::Internal, parent,
        TRANSPORT_DUMP_TYPE_IDENTIFIER),
    EntryIfaces(bus, objPath.c_str(), EntryIfaces::action::defer_emit)
{
    sourceDumpId(INVALID_SOURCE_ID);
}

} // namespace openpower::dump::host::system
