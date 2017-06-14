#pragma once

#include <experimental/filesystem>
#include <unistd.h>

namespace phosphor
{
namespace dump
{

namespace fs = std::experimental::filesystem;

/* Need a custom deleter for freeing up sd_event */
struct EventDeleter
{
    void operator()(sd_event* event) const
    {
        event = sd_event_unref(event);
    }
};
using EventPtr = std::unique_ptr<sd_event, EventDeleter>;

/** @struct CustomFd
 *
 *  RAII wrapper for file descriptor.
 */
struct CustomFd
{
    private:
        /** @brief File descriptor */
        int fd = -1;

    public:
        CustomFd() = delete;
        CustomFd(const CustomFd&) = delete;
        CustomFd& operator=(const CustomFd&) = delete;
        CustomFd(CustomFd&&) = delete;
        CustomFd& operator=(CustomFd&&) = delete;

        /** @brief Saves File descriptor and uses it to do file operation
          *
          *  @param[in] fd - File descriptor
          */
        CustomFd(int fd) : fd(fd) {}

        ~CustomFd()
        {
            if (fd >= 0)
            {
                close(fd);
            }
        }

        int operator()() const
        {
            return fd;
        }
};

} // namespace dump
} // namespace phosphor
