// A basic unit test that runs on a BMC (qemu or hardware)

#include <getopt.h>
#include <iostream>
#include <sstream>
#include "xyz/openbmc_project/Common/error.hpp"
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

const char *usage = "Usage: elog-report [OPTION]           \n\n\
Options:                                                     \n\
[NONE]                          Default test case.           \n\
-h, --help                      Display this usage text.     \n\
-r, --report <string>           Report desired error.      \n\n\
Valid errors to report:                                      \n\
InternalFailure                                              \n";

void reportError(const char *text)
{
    if (strcmp(text, "InternalFailure") == 0)
    {
        report<InternalFailure>();
    }

    return;
}

int main(int argc, char *argv[])
{
    char arg;

    if (argc == 1)
    {
        std::cerr << usage;
        return 1;
    }

    static struct option long_options[] =
    {
          {"help",    no_argument,       0, 'h'},
          {"report",  required_argument, 0, 'r'},
          {0, 0, 0, 0}
    };
    int option_index = 0;

    while((arg=getopt_long(argc,argv,"hr:",long_options,&option_index)) != -1)
    {
        switch (arg)
        {
            case 'r':
                reportError(optarg);
                return 0;
            case 'h':
            case '?':
                std::cerr << usage;
                return 1;

        }
    }

    return 0;

}
