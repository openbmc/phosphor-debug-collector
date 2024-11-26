#include "config.h"

#include "dump_offload.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <dump_utils.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Common/File/error.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <fstream>
#include <span>

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
    ssize_t numOfBytesWrote = 0;

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
            lg2::error("writeOnUnixSocket: select() failed, errno: {ERRNO}",
                       "ERRNO", errno);
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
                lg2::error("writeOnUnixSocket: write() failed, errno: {ERRNO}",
                           "ERRNO", errno);
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
        lg2::error("UNIX socket path too long");
        std::string msg =
            "UNIX socket path is too long " + std::string(strerror(errno));
        throw std::length_error(msg);
    }

    std::span<char> sunPathSpan(reinterpret_cast<char*>(socketAddr.sun_path),
                                sizeof(socketAddr.sun_path));
    strncpy(sunPathSpan.data(), sockPath.c_str(), sunPathSpan.size() - 1);
    sunPathSpan[sunPathSpan.size() - 1] = '\0'; // Ensure null-termination

    if ((unixSocket = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
    {
        lg2::error("socketInit: socket() failed, errno: {ERRNO}", "ERRNO",
                   errno);
        std::string msg = "socket() failed " + std::string(strerror(errno));
        throw std::runtime_error(msg);
    }
    if (bind(unixSocket, (struct sockaddr*)&socketAddr, sizeof(socketAddr)) ==
        -1)
    {
        lg2::error("socketInit: bind() failed, errno: {ERRNO}", "ERRNO", errno);
        close(unixSocket);
        std::string msg = "socket bind failed " + std::string(strerror(errno));
        throw std::runtime_error(msg);
    }
    if (listen(unixSocket, 1) == -1)
    {
        lg2::error("socketInit: listen() failed, errno: {ERRNO}", "ERRNO",
                   errno);
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
            lg2::error("select() failed, errno: {ERRNO}, DUMP_ID: {DUMP_ID}",
                       "ERRNO", errno, "DUMP_ID", dumpId);
            std::string msg = "select() failed " + std::string(strerror(errno));
            throw std::runtime_error(msg);
        }
        else if ((retVal > 0) && (FD_ISSET(unixSocket(), &readFD)))
        {
            CustomFd socketFD = accept(unixSocket(), NULL, NULL);
            if (socketFD() < 0)
            {
                lg2::error(
                    "accept() failed, errno: {ERRNO}, DUMP_ID: {DUMP_ID}",
                    "ERRNO", errno, "DUMP_ID", dumpId);
                std::string msg =
                    "accept() failed " + std::string(strerror(errno));
                throw std::runtime_error(msg);
            }

            std::ifstream infile{file, std::ios::in | std::ios::binary};
            if (!infile.good())
            {
                // Unable to open the dump file
                lg2::error("Failed to open the dump from file, errno: {ERRNO}, "
                           "DUMPFILE: {DUMP_FILE}, DUMP_ID: {DUMP_ID}",
                           "ERRNO", errno, "DUMP_FILE", file, "DUMP_ID",
                           dumpId);
                elog<Open>(ErrnoOpen(errno), PathOpen(file.c_str()));
            }

            infile.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                              std::ifstream::eofbit);

            lg2::info("Opening File for RW, FILENAME: {FILENAME}", "FILENAME",
                      file.filename().c_str());

            std::filebuf* pbuf = infile.rdbuf();

            // get file size using buffer's members
            std::size_t size = pbuf->pubseekoff(0, infile.end, infile.in);
            pbuf->pubseekpos(0, infile.in);

            // allocate memory to contain file data
            std::unique_ptr<char[]> buffer(new char[size]);
            // get file data
            // cast size to std::streamsize for further operations
            std::streamsize streamSize = static_cast<std::streamsize>(size);
            pbuf->sgetn(buffer.get(), streamSize);
            infile.close();

            writeOnUnixSocket(socketFD(), buffer.get(), size);
        }
    }
    catch (const std::ifstream::failure& oe)
    {
        std::remove(writePath.c_str());
        auto err = errno;
        lg2::error("Failed to open, errormsg: {ERROR}, "
                   "OPENINTERFACE: {OPEN_INTERFACE}, DUMP_ID: {DUMP_ID}",
                   "ERROR", oe, "OPEN_INTERFACE", file, "DUMP_ID", dumpId);
        elog<Open>(ErrnoOpen(err), PathOpen(file.c_str()));
    }
    catch (const std::exception& e)
    {
        std::remove(writePath.c_str());
        auto err = errno;
        lg2::error("Failed to offload dump, errormsg: {ERROR}, "
                   "DUMPFILE: {DUMP_FILE}, DUMP_ID: {DUMP_ID}",
                   "ERROR", e, "DUMP_FILE", writePath, "DUMP_ID", dumpId);
        elog<Write>(ErrnoWrite(err), PathWrite(writePath.c_str()));
    }
    std::remove(writePath.c_str());
    return;
}

} // namespace offload
} // namespace dump
} // namespace phosphor
