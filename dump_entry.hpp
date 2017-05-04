#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include "xyz/openbmc_project/Dump/Entry/server.hpp"
#include "xyz/openbmc_project/Object/Delete/server.hpp"
#include "xyz/openbmc_project/Time/EpochTime/server.hpp"

namespace phosphor
{
namespace dump
{

template <typename T>
using ServerObject = typename sdbusplus::server::object::object<T>;

using EntryIfaces = sdbusplus::server::object::object <
                    sdbusplus::xyz::openbmc_project::Dump::server::Entry,
                    sdbusplus::xyz::openbmc_project::Object::server::Delete,
                    sdbusplus::xyz::openbmc_project::Time::server::EpochTime >;

/** @class Entry
 *  @brief OpenBMC Dump Entry implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Entry DBus API
 */
class Entry : public EntryIfaces
{
    public:
        Entry() = delete;
        Entry(const Entry&) = delete;
        Entry& operator=(const Entry&) = delete;
        Entry(Entry&&) = delete;
        Entry& operator=(Entry&&) = delete;
        virtual ~Entry() = default;

        /** @brief Constructor for the Dump Entry Object
         *  @param[in] bus - Bus to attach to.
         *  @param[in] obj - Object path to attach to
         */
        Entry(sdbusplus::bus::bus& bus, const char* obj);

        /** @brief Delete this d-bus object.
         */
        void delete_() override ;

};

} // namespace dump
} // namespace phosphor
