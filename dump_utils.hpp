#pragma once

#include <systemd/sd-event.h>
#include <unistd.h>

#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/State/Boot/Progress/server.hpp>
#include <xyz/openbmc_project/State/Host/server.hpp>

#include <memory>

namespace phosphor
{
namespace dump
{

using BootProgress = sdbusplus::xyz::openbmc_project::State::Boot::server::
    Progress::ProgressStages;
using HostState =
    sdbusplus::xyz::openbmc_project::State::server::Host::HostState;

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
    CustomFd(int fd) : fd(fd)
    {}

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

/**
 * @brief Get the bus service
 *
 * @param[in] bus - Bus to attach to.
 * @param[in] path - D-Bus path name.
 * @param[in] interface - D-Bus interface name.
 * @return the bus service as a string
 **/
std::string getService(sdbusplus::bus::bus& bus, const std::string& path,
                       const std::string& interface);

/**
 * @brief Get the host state
 *
 * @return HostState on success
 *         Throw exception on failure
 *
 */
HostState getHostState();

/**
 * @brief Get the host boot progress stage
 *
 * @return BootProgress on success
 *         Throw exception on failure
 *
 */
BootProgress getBootProgress();

/**
 * @brief Get the host state value
 *
 * @param[in] intf - Interface to get the value
 * @param[in] objPath - Object path of the service
 * @param[in] state - State name to get
 *
 * @return The state value on success
 *         Throw exception on failure
 */
std::string getStateValue(std::string intf, std::string objPath,
                          std::string state);

/**
 * @brief Check whether host is running
 *
 * @return true if the host running else false.
 *         Throw exception on failure.
 */
bool isHostRunning();

/**
 * @brief Check whether host is quiesced
 *
 * @return true if the host is quiesced else false.
 *         Throw exception on failure.
 */
bool isHostQuiesced();

} // namespace dump
} // namespace phosphor
