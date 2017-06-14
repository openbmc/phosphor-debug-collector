#pragma once

#include <experimental/filesystem>

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

using EntryIfaces = sdbusplus::server::object::object<
                    sdbusplus::xyz::openbmc_project::Dump::server::Entry,
                    sdbusplus::xyz::openbmc_project::Object::server::Delete,
                    sdbusplus::xyz::openbmc_project::Time::server::EpochTime>;

namespace fs = std::experimental::filesystem;

class Manager;

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
        ~Entry() = default;

        /** @brief Constructor for the Dump Entry Object
         *  @param[in] bus - Bus to attach to.
         *  @param[in] objPath - Object path to attach to
         *  @param[in] dumpId - Dump id.
         *  @param[in] timeStamp - Dump creation timestamp
         *             since the epoch.
         *  @param[in] fileSize - Dump file size in bytes.
         *  @param[in] file - Dump file name.
         *  @param[in] parent - The dump entry's parent.
         */
        Entry(sdbusplus::bus::bus& bus,
              const std::string& objPath,
              uint32_t dumpId,
              uint64_t timeStamp,
              uint64_t fileSize,
              const fs::path& file,
              Manager& parent):
            EntryIfaces(bus, objPath.c_str(), true),
            file(file),
            parent(parent),
            id(dumpId)
        {
            size(fileSize);
            elapsed(timeStamp);
            // Emit deferred signal.
            this->emit_object_added();
        };

        /** @brief Delete this d-bus object.
         */
        void delete_() override ;

    private:
        /** @Dump file name */
        fs::path file;

        /** @brief This entry's parent */
        Manager& parent;

        /** @brief This entry's id */
        uint32_t id;
};

} // namespace dump
} // namespace phosphor
