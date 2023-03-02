#pragma once

#include <map>
#include <string>
#include <variant>

namespace phosphor
{
namespace dump
{
using DumpCreateParams =
    std::map<std::string, std::variant<std::string, uint64_t>>;
}
} // namespace phosphor
