#pragma once

//#include "host_interface.hpp"

#include <libpldm/pldm.h>

#include <memory>
#include <sdeventplus/source/io.hpp>

namespace phosphor
{
namespace dump
{
namespace pldm
{

enum class CmdStatus
{
    success,
    failure
};

/**
 * @class PLDMInterface
 *
 * This class handles sending the SetNumericEffecterValue PLDM
 * command to the host to start dump offload.
 *
 */
class PLDMInterface
{
  public:
    PLDMInterface(const PLDMInterface&) = default;
    PLDMInterface& operator=(const PLDMInterface&) = default;
    PLDMInterface(PLDMInterface&&) = default;
    PLDMInterface& operator=(PLDMInterface&&) = default;

    /**
     * @brief Constructor
     */
    PLDMInterface()
    {
        readEID();
    }

    /**
     * @brief Destructor
     */
    ~PLDMInterface();

    /**
     * @brief Kicks of the SetNumericEffecterValue command to
     *        start offload the dump
     *
     * @param[in] id - The Dump Source ID.
     *
     * @return CmdStatus - the success/fail status of the send
     */

    CmdStatus sendGetSysDumpCmd(uint32_t id);

    bool _inProgress = false;

  private:
    /**
     * @brief Reads the MCTP endpoint ID out of a file
     */
    void readEID();

    /**
     * @brief Opens the PLDM file descriptor
     */
    void open();

    /**
     * @brief Reads the PLDM instance ID to use for the upcoming
     *        command.
     */
    void readInstanceID();

    /**
     * @brief Encodes and sends the SetNumericEffecterValue cmd
     *
     * @param[in] id - The PEL ID
     */
    void doSendRcv(uint32_t id);

    /**
     * @brief Closes the PLDM file descriptor
     */
    void closeFD();

    uint8_t getPLDMInstanceID(uint8_t eid) const;
    /**
     * @brief The MCTP endpoint ID
     */
    mctp_eid_t _eid;

    /**
     * @brief The PLDM instance ID of the current command
     */
    uint8_t _instanceID;

    /**
     * @brief The PLDM command file descriptor for the current command
     */
    int _fd = -1;
};

} // namespace pldm
} // namespace dump
} // namespace phosphor
