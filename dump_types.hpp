#pragma once

#include <string>
#include <unordered_map>

namespace phosphor
{
namespace dump
{
// Overall dump category for example BMC dump
using DUMP_CATEGORY = std::string;

// Dump type
using DUMP_TYPE = std::string;

// Dump collection indicator
using DUMP_COLLECTION_TYPE = std::string;

// A table of dump types
using DUMP_TYPE_TABLE =
    std::unordered_map<DUMP_TYPE,
                       std::pair<DUMP_COLLECTION_TYPE, DUMP_CATEGORY>>;

extern DUMP_TYPE_TABLE dumpTypeTable;
} // namespace dump
} // namespace phosphor
