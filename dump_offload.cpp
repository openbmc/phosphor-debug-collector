#include "config.h"

#include "dump_offload.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <xyz/openbmc_project/Common/File/error.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace dump
{
namespace offload
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

bool writeOnUnixSocket(const int sock, const char* buf,
                       const uint64_t blockSize)
{
    int nwrite = 0;
    for (uint64_t i = 0; i < blockSize; i = i + nwrite)
    {

        fd_set wfd;
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        FD_ZERO(&wfd);
        FD_SET(sock, &wfd);
        int nfd = sock + 1;

        int retval = select(nfd, NULL, &wfd, NULL, &tv);
        if ((retval < 0) && (errno != EINTR))
        {
            std::cout << "\nselect call failed " << errno << std::endl;
            return true;
        }
        if (retval == 0)
        {
            std::cout << "\nUNIX fd is not ready to write";
            nwrite = 0;
            continue;
        }
        if ((retval > 0) && (FD_ISSET(sock, &wfd)))
        {
            nwrite = write(sock, buf + i, blockSize - i);

            if (nwrite < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                {
                    std::cout << "Write call failed with EAGAIN or EWOULDBLOCK "
                                 "or EINTR";
                    nwrite = 0;
                    continue;
                }
                std::cout << "Failed to write";
                return true;
            }
        }
        else
        {
            nwrite = 0;
            std::cout << "fd is not ready for write";
        }
    }
    return false;
}

void requestOffload(fs::path file, uint32_t dumpId, std::string writePath)
{
    using namespace sdbusplus::xyz::openbmc_project::Common::File::Error;
    using ErrnoWrite = xyz::openbmc_project::Common::File::Write::ERRNO;
    using PathWrite = xyz::openbmc_project::Common::File::Write::PATH;

    // open a dump file for a transfer.
    fs::path dumpPath(BMC_DUMP_PATH);
    dumpPath /= std::to_string(dumpId);
    dumpPath /= file.filename();

    int sock;
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (strnlen(writePath.c_str(), sizeof(addr.sun_path)) ==
        sizeof(addr.sun_path))
    {
        log<level::ERR>("UNIX socket path too long");
        return;
    }

    strncpy(addr.sun_path, writePath.c_str(), sizeof(addr.sun_path) - 1);

    if ((sock = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
    {
        log<level::ERR>("SOCKET failed");
        return;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        log<level::ERR>("CONNECT failed");
        close(sock);
        return;
    }

    std::ifstream infile{dumpPath, std::ios::in | std::ios::binary};
    if (!infile.good())
    {
        // Unable to open the dump file
        log<level::ERR>("Failed to open the dump from file ",
                        entry("ERR=%d", errno),
                        entry("DUMPFILE=%s", dumpPath.c_str()));
        elog<InternalFailure>();
    }

    infile.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                      std::ifstream::eofbit);

    try
    {
        log<level::INFO>("Opening File for RW ",
                         entry("FILENAME=%s", file.filename().c_str()));

        std::filebuf* pbuf = infile.rdbuf();

        // get file size using buffer's members
        std::size_t size = pbuf->pubseekoff(0, infile.end, infile.in);
        pbuf->pubseekpos(0, infile.in);

        // allocate memory to contain file data
        char* buffer = new char[size];
        // get file data
        pbuf->sgetn(buffer, size);
        infile.close();

        bool status = writeOnUnixSocket(sock, buffer, size);
        if (status)
        {

            std::cerr << "Failed to write on socket" << std::endl;
            elog<InternalFailure>();
            return;
        }
        close(sock);
        std::cerr << "closing socket" << std::endl;
    }
    catch (std::ofstream::failure& oe)
    {
        auto err = errno;
        log<level::ERR>("Failed to write to write interface ",
                        entry("ERR=%s", oe.what()),
                        entry("WRITEINTERFACE=%s", writePath.c_str()));
        elog<Write>(ErrnoWrite(err), PathWrite(writePath.c_str()));
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Failed get the dump from file ",
                        entry("ERR=%s", e.what()),
                        entry("DUMPFILE=%s", dumpPath.c_str()));
        elog<InternalFailure>();
    }
}

} // namespace offload
} // namespace dump
} // namespace phosphor
