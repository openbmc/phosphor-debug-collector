#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include "xyz/openbmc_project/Dump/Entry/server.hpp"
#include "xyz/openbmc_project/Object/Delete/server.hpp"
#include "xyz/openbmc_project/Time/EpochTime/server.hpp"
#include <experimental/filesystem>

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
         *  @param[in] objpath - Object path to attach to
         *  @param[in] dumpid - Dump id.
         *  @param[in] timestamp - Dump creation timestamp since 1970.
         *  @param[in] filesize - Dump file size in bytes.
         *  @param[in] file - Dump file name.
         *  @param[in] parent - The dump entry's parent.
         */
        Entry(sdbusplus::bus::bus& bus,
              const std::string& objpath,
              uint32_t dumpid,
              uint64_t timestamp,
              uint64_t filesize,
              const fs::path& file,
              Manager& parent):
            EntryIfaces(bus, objpath.c_str()),
            dumpFile(file),
            parent(parent),
            id(dumpid)
        {
            size(filesize);
            elapsed(timestamp);
        };

        /** @brief Delete this d-bus object.
         */
        void delete_() override ;

    private:
        /** @Dump file name */
        fs::path dumpFile;

        /** @brief This entry's parent */
        Manager& parent;

        /** @brief This entry's id */
        uint32_t id;
};

} // namespace dump
} // namespace phosphor
