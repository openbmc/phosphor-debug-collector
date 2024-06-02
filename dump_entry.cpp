#include "dump_entry.hpp"

#include "dump_manager.hpp"

#include <fcntl.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>
#include <xyz/openbmc_project/Common/File/error.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <cstring>

namespace phosphor
{
namespace dump
{

using namespace phosphor::logging;

void Entry::delete_()
{
    // Remove Dump entry D-bus object
    parent.erase(id);
}

sdbusplus::message::unix_fd Entry::getFileHandle()
{
    using namespace sdbusplus::xyz::openbmc_project::Common::File::Error;
    using metadata = xyz::openbmc_project::Common::File::Open;
    if (file.empty())
    {
        lg2::error("Failed to get file handle: File path is empty.");
        elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
    }

    if (fdCloseEventSource)
    {
        // Return the existing file descriptor
        return fdCloseEventSource->first;
    }

    int fd = open(file.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd == -1)
    {
        auto err = errno;
        lg2::error("Failed to open dump file: id: {ID} error: {ERRNO}", "ID",
                   id, "ERRNO", std::strerror(errno));
        elog<Open>(metadata::ERRNO(err), metadata::PATH(file.c_str()));
    }

    // Create a new Defer event source for closing this fd
    sdeventplus::Event event = sdeventplus::Event::get_default();
    auto eventSource = std::make_unique<sdeventplus::source::Defer>(
        event, [this](auto& /*source*/) { closeFD(); });

    // Store the file descriptor and event source in the optional pair
    fdCloseEventSource = std::make_pair(fd, std::move(eventSource));

    return fd;
}

void Entry::serialize()
{
    // Folder for serialized entry
    std::filesystem::path dir = file.parent_path() / PRESERVE;

    // Serialized entry file
    std::filesystem::path serializePath = dir / SERIAL_FILE;
    try
    {
        if (!std::filesystem::exists(dir))
        {
            std::filesystem::create_directories(dir);
        }

        std::ofstream os(serializePath, std::ios::binary);
        if (!os.is_open())
        {
            lg2::error("Failed to open file for serialization: {PATH} ", "PATH",
                       serializePath);
            return;
        }
        nlohmann::json j;
        j["version"] = CLASS_SERIALIZATION_VERSION;
        j["dumpId"] = id;
        j["originatorId"] = originatorId();
        j["originatorType"] = originatorType();
        j["startTime"] = startTime();

        os << j.dump();
    }
    catch (const std::exception& e)
    {
        lg2::error("Serialization error: {PATH} {ERROR} ", "PATH",
                   serializePath, "ERROR", e);

        // Remove the serialization folder if that got created
        // Ignore the error since folder may not be created
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }
}

void Entry::deserialize(const std::filesystem::path& dumpPath)
{
    try
    {
        // .preserve folder
        std::filesystem::path dir = dumpPath / PRESERVE;
        if (!std::filesystem::exists(dir))
        {
            lg2::info("Serialization directory: {SERIAL_DIR} doesnt exist, "
                      "skip deserialization",
                      "SERIAL_DIR", dir);
            return;
        }

        // Serialized entry
        std::filesystem::path serializePath = dir / SERIAL_FILE;
        std::ifstream is(serializePath, std::ios::binary);
        if (!is.is_open())
        {
            lg2::error("Failed to open file for deserialization: {PATH}",
                       "PATH", serializePath);
            return;
        }
        nlohmann::json j;
        is >> j;

        uint32_t version;
        j.at("version").get_to(version);
        if (version == CLASS_SERIALIZATION_VERSION)
        {
            uint32_t storedId;
            j.at("dumpId").get_to(storedId);
            if (storedId == id)
            {
                originatorId(j["originatorId"].get<std::string>());
                originatorType(j["originatorType"].get<originatorTypes>());
                startTime(j["startTime"].get<uint64_t>());
            }
            else
            {
                lg2::error("The id ({ID_IN_FILE}) is not matching the dump id "
                           "({DUMPID}); skipping deserialization.",
                           "ID_IN_FILE", storedId, "DUMPID", id);

                // Id is not matching, this could be due to file corruption
                // deleting the .preserve folder.
                // Attempt to delete the folder and ignore any error.
                std::error_code ec;
                std::filesystem::remove_all(dir, ec);
            }
        }
        else
        {
            lg2::error("The serialized file version and current class version"
                       "doesnt match, skip deserialization {VERSION}",
                       "VERSION", version);
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Deserialization error: {PATH}, {ERROR}", "PATH", dumpPath,
                   "ERROR", e);
    }
}

} // namespace dump
} // namespace phosphor
