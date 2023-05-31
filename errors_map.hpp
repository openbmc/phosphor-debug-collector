#pragma once

#include <map>
#include <string>
#include <vector>

using EType = std::string;
using Error = std::string;
using ErrorList = std::vector<Error>;
using ErrorMap = std::map<EType, std::vector<Error>>;

extern const ErrorMap errorMap;
