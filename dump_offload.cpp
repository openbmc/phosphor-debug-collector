#include "dump_offload.hpp"
#include "dump_utils.hpp"

#include <xyz/openbmc_project/Common/File/error.hpp>

namespace phosphor
{
namespace dump
{
namespace offload
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

void requestOffload(fs::path file, uint32_t dumpId, std::string writePath)
{
    using namespace sdbusplus::xyz::openbmc_project::Common::File::Error;
    using ErrnoOpen = xyz::openbmc_project::Common::File::Open::ERRNO;
    using PathOpen = xyz::openbmc_project::Common::File::Open::PATH;
    using ErrnoWrite = xyz::openbmc_project::Common::File::Write::ERRNO;
    using PathWrite = xyz::openbmc_project::Common::File::Write::PATH;

    constexpr auto flags = O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE;
    constexpr auto permission = 0666;

    // Open the device for file transfer
    CustomFd fd = open(writePath.c_str(), flags, permission);
    if (fd() == -1)
    {
        auto err = errno;
        log<level::ERR>("Write device path does not exist at ",
                        entry("WRITEINTERFACE=%s", writePath.c_str()));
        elog<Open>(ErrnoOpen(err), PathOpen(writePath.c_str()));
    }
    log<level::INFO>("Write interface open() successfull ",
                     entry("WRITEINTERFACE=%s", writePath.c_str()));

    // open a dump file for a transfer.
    std::streampos size;
    std::stringstream dumpStream;
    fs::path dumpPath(BMC_DUMP_PATH);
    dumpPath /= std::to_string(dumpId);
    dumpPath /= file.filename();

    std::ifstream infile;
    infile.exceptions(std::ifstream::failbit |
                      std::ifstream::badbit |
                      std::ifstream::eofbit);

    try
    {
        log<level::INFO>("Opening File for RW ",
                         entry("FILENAME=%s", file.filename().c_str()));
        infile.open(dumpPath, std::ios::in | std::ios::binary);
        size = infile.tellg();
        dumpStream << infile.rdbuf();
        infile.close();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Failed get the dump from file ",
                        entry("ERR=%s", e.what()),
                        entry("DUMPFILE=%s", dumpPath.c_str()));
        elog<InternalFailure>();
    }

    int rc = write(fd(), dumpStream.rdbuf(), size);
    if (rc == -1)
    {
        auto err = errno;
        log<level::ERR>("Failed to write to nbd device ",
                        entry("RC=%d", rc));
        elog<Write>(ErrnoWrite(err), PathWrite(writePath.c_str()));
    }
    log<level::INFO>("Dump write successfull ", entry("RC=%d", rc));
}

} // namespace offload
} // namespace dump
} // namespace phosphor
