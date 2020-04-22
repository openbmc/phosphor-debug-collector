#include "config.h"

#include "dump_offload.hpp"

#include <fstream>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

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
    // open a dump file for a transfer.
    fs::path dumpPath(BMC_DUMP_PATH);
    dumpPath /= std::to_string(dumpId);
    dumpPath /= file.filename();

    std::ifstream infile;
    std::ofstream outfile;
    infile.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                      std::ifstream::eofbit);
    outfile.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                       std::ifstream::eofbit);

    try
    {
        log<level::INFO>("Opening File for RW ",
                         entry("FILENAME=%s", file.filename().c_str()));
        infile.open(dumpPath, std::ios::in | std::ios::binary);
        outfile.open(writePath, std::ios::out);
        outfile << infile.rdbuf() << std::flush;
        infile.close();
        outfile.close();
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
