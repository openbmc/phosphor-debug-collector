#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdeventplus/source/child.hpp>
#include "dump_utils.hpp"
#include <sdeventplus/source/event.hpp>
#include <sdeventplus/exception.hpp>
#include <sdeventplus/source/base.hpp>

#include <filesystem>
#include <functional>
#include <map>
#include <memory>

namespace phosphor
{
namespace dump
{
using ::sdeventplus::source::Child;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

class DumpCollector
{
  public:
    DumpCollector() = delete;
    DumpCollector(const DumpCollector&) = default;
    DumpCollector& operator=(const DumpCollector&) = delete;
    DumpCollector(DumpCollector&&) = delete;
    DumpCollector& operator=(DumpCollector&&) = delete;
    virtual ~DumpCollector() = default;

    DumpCollector(const EventPtr& event): eventLoop(event.get())
    {}

        /**
         * @brief Execute the dump capture process.
         *
         * @param commandAndArgs The command to be executed to collect the dump
         * @param userCallback The specialized method to be called as part main
         * callback
         * @return The increment of the last entry id.
         */
        void collectDump(const std::vector<std::string>& commandAndArgs,
                         std::function<void()> userCallback)
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            std::vector<const char*> cArgs;
            for (const auto& arg : commandAndArgs)
            {
                cArgs.push_back(arg.c_str());
            }
            cArgs.push_back(nullptr);
            execv(cArgs[0], const_cast<char* const*>(cArgs.data()));

            // dreport script execution is failed.
            auto error = errno;
            lg2::error("Error occurred during dreport function execution, "
                       "errno: {ERRNO}",
                       "ERRNO", error);
            elog<InternalFailure>();
        }
        else if (pid > 0)
        {
            Child::Callback callback =
                [this, userCallback, pid](Child&, const siginfo_t*) {
                if (userCallback)
                {
                    userCallback();
                }
                this->childPtrMap.erase(pid);
            };
            try
            {
                childPtrMap.emplace(
                    pid, std::make_unique<Child>(eventLoop.get(), pid,
                                                 WEXITED | WSTOPPED,
                                                 std::move(callback)));
            }
            catch (const sdeventplus::SdEventError& ex)
            {
                // Failed to add to event loop
                lg2::error(
                    "Error occurred during the sdeventplus::source::Child creation "
                    "ex: {ERROR}",
                    "ERROR", ex);
                elog<InternalFailure>();
            }
        }
        else
        {
            auto error = errno;
            lg2::error("Error occurred during fork, errno: {ERRNO}", "ERRNO",
                       error);
            elog<InternalFailure>();
        }
        return;
    }

  private:
    /** sdbusplus Dump event loop */
    EventPtr eventLoop;

    /** @brief map of SDEventPlus child pointer added to event loop */
    std::map<pid_t, std::unique_ptr<Child>> childPtrMap;
};
} // namespace dump
} // namespace phosphor
