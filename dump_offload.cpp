#include "config.h"

#include "dump_offload.hpp"

#include <fmt/core.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <dump_utils.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <xyz/openbmc_project/Common/File/error.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <fstream>

namespace phosphor
{
namespace dump
{
namespace offload
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

/** @brief API to write data on unix socket.
 *
 * @param[in] socket     - unix socket
 * @param[in] buf        - buffer
 * @param[in] blockSize  - size of data
 *
 * @return  void
 */
void writeOnUnixSocket(const int socket, const char* buf,
                       const uint64_t blockSize)
{
    int numOfBytesWrote = 0;

    for (uint64_t i = 0; i < blockSize; i = i + numOfBytesWrote)
    {
        numOfBytesWrote = 0;
        fd_set writeFileDescriptor;
        struct timeval timeVal;
        timeVal.tv_sec = 5;
        timeVal.tv_usec = 0;

        FD_ZERO(&writeFileDescriptor);
        FD_SET(socket, &writeFileDescriptor);
        int nextFileDescriptor = socket + 1;

        int retVal = select(nextFileDescriptor, NULL, &writeFileDescriptor,
                            NULL, &timeVal);
        if (retVal <= 0)
        {
            log<level::ERR>(
                fmt::format("writeOnUnixSocket: select() failed, errno({})",
                            errno)
                    .c_str());
            std::string msg = "select() failed " + std::string(strerror(errno));
            throw std::runtime_error(msg);
        }
        if ((retVal > 0) && (FD_ISSET(socket, &writeFileDescriptor)))
        {
            numOfBytesWrote = write(socket, buf + i, blockSize - i);
            if (numOfBytesWrote < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                {
                    numOfBytesWrote = 0;
                    continue;
                }
                log<level::ERR>(
                    fmt::format("writeOnUnixSocket: write() failed, errno({})",
                                errno)
                        .c_str());
                std::string msg =
                    "write() on socket failed " + std::string(strerror(errno));
                throw std::runtime_error(msg);
            }
        }
    }
    return;
}

/**@brief API to setup unix socket.
 *
 * @param[in] sockPath  - unix socket path
 *
 * @return returns returns socket fd on success
 *                 and on error exception will be thrown
 */
int socketInit(const std::string& sockPath)
{
    int unixSocket;
    struct sockaddr_un socketAddr;
    memset(&socketAddr, 0, sizeof(socketAddr));
    socketAddr.sun_family = AF_UNIX;
    if (strnlen(sockPath.c_str(), sizeof(socketAddr.sun_path)) ==
        sizeof(socketAddr.sun_path))
    {
        log<level::ERR>("UNIX socket path too long");
        std::string msg =
            "UNIX socket path is too long " + std::string(strerror(errno));
        throw std::length_error(msg);
    }
    strncpy(socketAddr.sun_path, sockPath.c_str(),
            sizeof(socketAddr.sun_path) - 1);
    if ((unixSocket = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
    {
        log<level::ERR>(
            fmt::format("socketInit: socket() failed, errno({})", errno)
                .c_str());
        std::string msg = "socket() failed " + std::string(strerror(errno));
        throw std::runtime_error(msg);
    }
    if (bind(unixSocket, (struct sockaddr*)&socketAddr, sizeof(socketAddr)) ==
        -1)
    {
        log<level::ERR>(
            fmt::format("socketInit: bind() failed, errno({})", errno).c_str());
        close(unixSocket);
        std::string msg = "socket bind failed " + std::string(strerror(errno));
        throw std::runtime_error(msg);
    }
    if (listen(unixSocket, 1) == -1)
    {
        log<level::ERR>(
            fmt::format("socketInit: listen() failed, errno({})", errno)
                .c_str());
        close(unixSocket);
        std::string msg = "listen() failed " + std::string(strerror(errno));
        throw std::runtime_error(msg);
    }
    return unixSocket;
}

void requestOffload(std::filesystem::path file, uint32_t dumpId,
                    std::string writePath)
{
    using namespace sdbusplus::xyz::openbmc_project::Common::File::Error;
    using ErrnoOpen = xyz::openbmc_project::Common::File::Open::ERRNO;
    using PathOpen = xyz::openbmc_project::Common::File::Open::PATH;
    using ErrnoWrite = xyz::openbmc_project::Common::File::Write::ERRNO;
    using PathWrite = xyz::openbmc_project::Common::File::Write::PATH;
    // open a dump file for a transfer.
    std::filesystem::path dumpPath(BMC_DUMP_PATH);
    dumpPath /= std::to_string(dumpId);
    dumpPath /= file.filename();

    try
    {

        CustomFd unixSocket = socketInit(writePath);

        fd_set readFD;
        struct timeval timeVal;
        timeVal.tv_sec = 1;
        timeVal.tv_usec = 0;

        FD_ZERO(&readFD);
        FD_SET(unixSocket(), &readFD);
        int numOfFDs = unixSocket() + 1;

        int retVal = select(numOfFDs, &readFD, NULL, NULL, &timeVal);
        if (retVal <= 0)
        {
            log<level::ERR>(
                fmt::format("select() failed, errno({}), DUMP_ID({})", errno,
                            dumpId)
                    .c_str());
            std::string msg = "select() failed " + std::string(strerror(errno));
            throw std::runtime_error(msg);
        }
        else if ((retVal > 0) && (FD_ISSET(unixSocket(), &readFD)))
        {
            CustomFd socketFD = accept(unixSocket(), NULL, NULL);
            if (socketFD() < 0)
            {
                log<level::ERR>(
                    fmt::format("accept() failed, errno({}), DUMP_ID({})",
                                errno, dumpId)
                        .c_str());
                std::string msg =
                    "accept() failed " + std::string(strerror(errno));
                throw std::runtime_error(msg);
            }

            std::ifstream infile{dumpPath, std::ios::in | std::ios::binary};
            if (!infile.good())
            {
                // Unable to open the dump file
                log<level::ERR>(
                    fmt::format("Failed to open the dump from file, errno({}), "
                                "DUMPFILE({}), DUMP_ID({})",
                                errno, dumpPath.c_str(), dumpId)
                        .c_str());
                elog<Open>(ErrnoOpen(errno), PathOpen(dumpPath.c_str()));
            }

            infile.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                              std::ifstream::eofbit);

            log<level::INFO>(fmt::format("Opening File for RW, FILENAME({})",
                                         file.filename().c_str())
                                 .c_str());

            std::filebuf* pbuf = infile.rdbuf();

            // get file size using buffer's members
            std::size_t size = pbuf->pubseekoff(0, infile.end, infile.in);
            pbuf->pubseekpos(0, infile.in);

            // allocate memory to contain file data
            std::unique_ptr<char[]> buffer(new char[size]);
            // get file data
            pbuf->sgetn(buffer.get(), size);
            infile.close();

            writeOnUnixSocket(socketFD(), buffer.get(), size);
        }
    }
    catch (std::ifstream::failure& oe)
    {
        std::remove(writePath.c_str());
        auto err = errno;
        log<level::ERR>(
            fmt::format(
                "Failed to open, errormsg({}), OPENINTERFACE({}), DUMP_ID({})",
                oe.what(), dumpPath.c_str(), dumpId)
                .c_str());
        elog<Open>(ErrnoOpen(err), PathOpen(dumpPath.c_str()));
    }
    catch (const std::exception& e)
    {
        std::remove(writePath.c_str());
        auto err = errno;
        log<level::ERR>(fmt::format("Failed to offload dump, errormsg({}), "
                                    "DUMPFILE({}), DUMP_ID({})",
                                    e.what(), writePath.c_str(), dumpId)
                            .c_str());
        elog<Write>(ErrnoWrite(err), PathWrite(writePath.c_str()));
    }
    std::remove(writePath.c_str());
    return;
}

} // namespace offload
} // namespace dump
} // namespace phosphor
