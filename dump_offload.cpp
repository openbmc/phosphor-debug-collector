#include "dump_offload.hpp"

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

    int flags = O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE;
    int fd = -1;

    // Open the device for file transfer
    if (fd == -1)
    {
        fd = open(writePath.c_str(), flags, 0666);
        if (fd == -1)
        {
            auto err = errno;
            log<level::ERR>("Write device path does not exist at ",
                            entry("WRITEINTERFACE=%s", writePath.c_str()));
            elog<Open>(ErrnoOpen(err), PathOpen(writePath.c_str()));
        }
        log<level::INFO>("Write interface open() successfull ",
                         entry("WRITEINTERFACE=%s", writePath.c_str()));

        // open a bmcdump file for a transfer.
        std::streampos size;
        fs::path dumpPath(BMC_DUMP_PATH);
        dumpPath /= std::to_string(dumpId);
        dumpPath /= file.filename();
        char* memblock = NULL;
        std::ifstream infile(dumpPath,
                             std::ios::in | std::ios::binary | std::ios::ate);
        if (infile.is_open())
        {
            log<level::INFO>("Opening File for RW ",
                             entry("FILENAME=%s", file.filename().c_str()));
            size = infile.tellg();
            memblock = new char[size];
            infile.seekg(0, std::ios::beg);
            infile.read(memblock, size);
            infile.close();
        }
        else
        {
            log<level::ERR>("Failed to open dump ",
                            entry("DUMPFILE=%s", dumpPath.c_str()));
            elog<InternalFailure>();
        }

        int rc = write(fd, memblock, size);
        if (rc == -1)
        {
            auto err = errno;
            log<level::ERR>("Failed to write to nbd device ",
                            entry("RC=%d", rc));
            close(fd);
            elog<Write>(ErrnoWrite(err), PathWrite(writePath.c_str()));
        }
        log<level::INFO>("Dump write successfull ", entry("RC=%d", rc));
        delete[] memblock;
        close(fd);
    }
}

} // namespace offload
} // namespace dump
} // namespace phosphor
