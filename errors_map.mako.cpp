## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.
// !!! WARNING: This is a GENERATED Code..Please do NOT Edit !!!
#include "errors_map.hpp"

const ErrorMap errorMap = {
% for key, errors in errDict.items():
    {"${key}", {
    % for error in errors:
        "${error}",
    % endfor
    }},
% endfor
};
