#include "argument.hpp"
#include "dump_offload.hpp"

#include <iostream>
#include <string>
static void ExitWithError(const char* err, char** argv)
{
    phosphor::dump::util::ArgumentParser::usage(argv);
    std::cerr << std::endl;
    std::cerr << "ERROR: " << err << std::endl;
    exit(EXIT_FAILURE);
}
int main(int argc, char** argv)
{
    // Read arguments.
    auto options = phosphor::dump::util::ArgumentParser(argc, argv);

    std::string idStr = std::move((options)["id"]);
    if (idStr.empty())
    {
        ExitWithError("Dump id is not provided", argv);
    }
    auto id = std::stoi(idStr);

    std::filesystem::path path = std::move((options)["path"]);
    if (path.empty())
    {
        ExitWithError("Dump file path not specified.", argv);
    }

    std::string uri = std::move((options)["uri"]);
    if (path.empty())
    {
        ExitWithError("Dump offload uri not specified.", argv);
    }

    try
    {
        phosphor::dump::offload::requestOffload(path, id, uri);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}
