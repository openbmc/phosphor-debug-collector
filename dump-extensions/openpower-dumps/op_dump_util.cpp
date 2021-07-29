#include "op_dump_util.hpp"

namespace openpower
{
namespace dump
{
namespace util
{

int callback(sd_event_source*, const siginfo_t*, void*)
{
    // No specific action required in
    // the sd_event_add_child callback.
    return 0;
}

void captureDump(uint32_t dumpId, size_t allowedSize, std::string& inputDir,
                 std::string& packageDir, std::string& dumpPrefix,
                 const phosphor::dump::EventPtr& event)
{
    std::string idStr;
    try
    {
        idStr = std::to_string(dumpId);
    }
    catch (std::exception& e)
    {
        log<level::ERR>(
            fmt::frormat("Error converting id({}) to string", id).c_str());
        elog<InternalFailure>();
    }
    auto dumpTempPath = std::filesystem::path(inputDir) / idStr;

    pid_t pid = fork();
    if (pid == 0)
    {
        std::filesystem::path dumpPath(packageDir);
        dumpPath /= idStr;
        execl("/usr/bin/opdreport", "opdreport", "-d", dumpPath.c_str(), "-i",
              idStr.c_str(), "-s", std::to_string(allowedSize).c_str(), "-q",
              "-v", "-p", dumpTempPath.c_str(), "-t", "opdump", "-n",
              dumpPrefix.c_str(), nullptr);

        // opdreport script execution is failed.
        auto error = errno;
        log<level::ERR>(
            fmt::format(
                "Dump capture: Error occurred during "
                "opdreport function execution, errno({}), dumpPrefix({}), "
                "dumpPath({}), dumpSourcePath({}), allowedSize({})",
                error, dumpPrefix.c_str(), dumpPath.c_str(),
                dumpTempPath.c_str(), allowedSize)
                .c_str());
        elog<InternalFailure>();
    }
    else if (pid > 0)
    {
        auto rc = sd_event_add_child(event.get(), nullptr, pid,
                                     WEXITED | WSTOPPED, callback, nullptr);
        if (0 > rc)
        {
            // Failed to add to event loop
            log<level::ERR>(fmt::format("Hostboot dump: Error occurred during "
                                        "the sd_event_add_child call, rc({})",
                                        rc)
                                .c_str());
            elog<InternalFailure>();
        }
    }
    else
    {
        auto error = errno;
        log<level::ERR>(
            fmt::format("Hostboot dump: Error occurred during fork, errno({})",
                        error)
                .c_str());
        elog<InternalFailure>();
    }
}

} // namespace util
} // namespace dump
} // namespace openpower
