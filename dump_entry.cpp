#include "dump_entry.hpp"

#include "dump_manager.hpp"

#include <fcntl.h>

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

void Entry::serialize(const std::filesystem::path& filePath)
{
    try
    {
        std::filesystem::path dir = filePath.parent_path();
        if (!std::filesystem::exists(dir))
        {
            std::filesystem::create_directories(dir);
        }

        std::ofstream os(filePath, std::ios::binary);
        if (!os.is_open())
        {
            lg2::error("Failed to open file for serialization: {PATH} ", "PATH",
                       filePath);
        }
        cereal::BinaryOutputArchive archive(os);
        archive(originatorId(), originatorType(), startTime());
    }
    catch (const std::exception& e)
    {
        lg2::error("Serialization error: {PATH} {ERROR} ", "PATH", filePath,
                   "ERROR", e);
    }
}

void Entry::deserialize(const std::filesystem::path& filePath)
{
    try
    {
        std::ifstream is(filePath, std::ios::binary);
        if (!is.is_open())
        {
            lg2::error("Failed to open file for deserialization: {PATH}",
                       "PATH", filePath);
        }
        cereal::BinaryInputArchive archive(is);
        std::string originId;
        originatorTypes originType;
        uint64_t startTimeValue;
        archive(originId, originType, startTimeValue);
        originatorId(originId);
        originatorType(originType);
        startTime(startTimeValue);
    }
    catch (const std::exception& e)
    {
        lg2::error("Deserialization error: {PATH}, {ERROR}", "PATH", filePath,
                   "ERROR", e);
    }
}

} // namespace dump
} // namespace phosphor
