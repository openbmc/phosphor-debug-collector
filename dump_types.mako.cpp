## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.
// !!! WARNING: This is a GENERATED Code..Please do NOT Edit !!!
#include "dump_types.hpp"
namespace phosphor
{
namespace dump
{
DUMP_TYPE_TABLE dumpTypeTable = {
% for item in DUMP_TYPE_TABLE:
  % for key, values in item.items():
    {"${key}", {"${values[0]}", "${values[1]}"}},
  % endfor
% endfor
};

} // namespace dump
} // namespace phosphor

