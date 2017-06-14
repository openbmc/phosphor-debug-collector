#pragma once

namespace phosphor
{
namespace dump
{

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
