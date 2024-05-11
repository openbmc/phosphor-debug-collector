#pragma once

#include "dump_utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <com/ibm/Dump/Create/common.hpp>

#include <optional>

namespace openpower::dump
{
using OpDumpTypes = sdbusplus::common::com::ibm::dump::Create::DumpType;

/**
 * @struct DumpParameters
 * @brief Holds parameters relevant to dump creation.
 *
 * This structure encapsulates all necessary parameters for creating a dump,
 * including optional and mandatory fields based on the type of dump.
 */
struct DumpParameters
{
    OpDumpTypes type;
    std::optional<std::string> vspString;
    std::optional<std::string> userChallenge;
    std::optional<uint64_t> eid;
    std::optional<uint64_t> fid;
    std::string originatorId;
    phosphor::dump::originatorTypes originatorType;
};

namespace util
{

/** @brief Check whether OpenPOWER dumps are enabled
 *
 * param[in] bus - D-Bus handle
 *
 * If the settings service is not running then considering as
 * the dumps are enabled.
 * @return true - if dumps are enabled, false - if dumps are not enabled
 */
bool isOPDumpsEnabled(sdbusplus::bus_t& bus);

using BIOSAttrValueType = std::variant<int64_t, std::string>;

/** @brief Read a BIOS attribute value
 *
 *  @param[in] attrName - Name of the BIOS attribute
 *  @param[in] bus - D-Bus handle
 *
 *  @return The value of the BIOS attribute as a variant of possible types
 *
 *  @throws sdbusplus::exception::SdBusError if failed to read the attribute
 */
BIOSAttrValueType readBIOSAttribute(const std::string& attrName,
                                    sdbusplus::bus_t& bus);

/** @brief Check whether a system is in progress or available to offload.
 *
 *  @param[in] bus - D-Bus handle
 *
 *  @return true - A dump is in progress or available to offload
 *          false - No dump in progress
 */
bool isSystemDumpInProgress(sdbusplus::bus_t& bus);

/**
 * @brief Safely extracts a parameter from a map with a given key.
 *
 * @tparam T The expected type of the parameter to be extracted.
 * @param[in] key The key to search for in the map.
 * @param[in] params The map from which the parameter should be extracted.
 * @return An optional containing the value if all checks pass, or an empty
 * optional.
 */
template <typename T>
inline std::optional<T>
    safeExtractParameter(const std::string& key,
                         const phosphor::dump::DumpCreateParams& params);

/**
 * @brief Extracts and constructs a DumpParameters structure from a set of
 * parameters.
 *
 * @param[in] params The map containing the parameters.
 * @return A constructed DumpParameters structure.
 */
openpower::dump::DumpParameters
    extractDumpParameters(const phosphor::dump::DumpCreateParams& params);

/**
 * @brief Throws a standardized invalid argument error.
 *
 * @param[in] argumentName The name of the argument that is invalid.
 * @param[in] errorDetail The value or reason why the argument is considered
 * invalid.
 */
inline void throwInvalidArgument(const std::string& argumentName,
                                 const std::string& errorDetail)
{
    using namespace phosphor::logging;
    using InvalidArgument =
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
    using Argument = xyz::openbmc_project::Common::InvalidArgument;

    // Throw the exception with standardized argument name and detail
    elog<InvalidArgument>(Argument::ARGUMENT_NAME(argumentName.c_str()),
                          Argument::ARGUMENT_VALUE(errorDetail.c_str()));
}
} // namespace util
} // namespace openpower::dump
