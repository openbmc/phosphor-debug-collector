#include "argument.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>

namespace phosphor
{
namespace dump
{
namespace util
{

ArgumentParser::ArgumentParser(int argc, char** argv)
{
    auto option = 0;
    while (-1 != (option = getopt_long(argc, argv, optionstr, options, NULL)))
    {
        if ((option == '?') || (option == 'h'))
        {
            usage(argv);
            exit(-1);
        }

        auto i = &options[0];
        while ((i->val != option) && (i->val != 0))
        {
            ++i;
        }

        if (i->val)
        {
            arguments[i->name] = (i->has_arg ? optarg : true_string);
        }
    }
}

const std::string& ArgumentParser::operator[](const std::string& opt)
{
    auto i = arguments.find(opt);
    if (i == arguments.end())
    {
        return empty_string;
    }
    else
    {
        return i->second;
    }
}

void ArgumentParser::usage(char** argv)
{
    std::cerr << "Usage: " << argv[0] << " [options]\n";
    std::cerr << "Options:\n";
    std::cerr << "    --help            Print this menu\n";
    std::cerr << "    --id              id of the dump\n";
    std::cerr << "    --path            path to dump file\n";
    std::cerr << "    --uri             URI to write the dump\n";
    std::cerr << std::flush;
}

const option ArgumentParser::options[] = {
    {"id", required_argument, nullptr, 'i'},
    {"path", required_argument, nullptr, 'p'},
    {"uri", required_argument, nullptr, 'u'},
    {"help", no_argument, nullptr, 'h'},
    {0, 0, 0, 0},
};

const char* ArgumentParser::optionstr = "ipuh?";

const std::string ArgumentParser::true_string = "true";
const std::string ArgumentParser::empty_string = "";

} // namespace util
} // namespace dump
} // namespace phosphor
