#include "dump_offload.hpp"

namespace phosphor
{
namespace dump
{
namespace offload
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

void requestOffload(fs::path file)
{
    static constexpr auto nbdInterface = "/dev/nbd1";
    int flags = O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE;
    int fd = -1;

    // Open the nbd device for file transfer
    if (fd == -1)
    {
        fd = open(nbdInterface, flags, 0666);
        if (fd == -1)
        {
            log<level::ERR>("NBD file does not exist at ", 
                            entry("nbdInterface = %s", nbdInterface));
            report<InternalFailure>();
            close(fd);
          
        }
        log<level::INFO>("NBD open() successfull ", 
                         entry("nbdInterface = %s", nbdInterface));

        // open a bmcdump file for a transfer.
        std::streampos size;
        fs::path dumpPath(BMC_DUMP_PATH);
        dumpPath /= std::to_string(3);
        dumpPath /= file.filename();
        char * memblock = NULL;
        std::ifstream infile (dumpPath, std::ios::in|std::ios::binary|std::ios::ate);
        std::cout << dumpPath << std::endl;
        if (infile.is_open())
        {
            log<level::INFO>("Opening File for RW ", 
                        entry("filename = %s", file.filename().c_str()));
            size = infile.tellg();
            memblock = new char [size];
            infile.seekg (0, std::ios::beg);
            infile.read (memblock, size);
            infile.close();
        }
   
        std::cout << size << std::endl;
        int rc = write(fd, memblock, size);
        if (rc == -1)
        {
            log<level::ERR>("Failed to write to nbd device ",
                    entry("RC=%d", rc));
            report<InternalFailure>();
            close(fd);
        }
        log<level::INFO>("Write() to nbd successfull ", 
                         entry("RC = %d", rc));
        std::cout << rc << std::endl;
        delete[] memblock;
        close(fd);
    }
}

} // namespace bmc
} // namespace dump
} // namespace phosphor
