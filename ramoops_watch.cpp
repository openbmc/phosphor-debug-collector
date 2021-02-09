#include "ramoops_watch.hpp"

#include "dump_internal.hpp"

namespace phosphor
{
namespace dump
{
namespace ramoops
{

void RamoopsWatch::collectRamoopsData()
{
    std::vector<std::string> paths{};
    for (auto& path : fs::directory_iterator(pstoreDir))
    {
        if (fs::is_directory(path))
        {
            continue;
        }

        paths.push_back(path.path());
    }

    if (paths.empty())
    {
        return;
    }

    iMgr.create(phosphor::dump::bmc::Type::ApplicationCored, paths);
}

void RamoopsWatch::watchCallback(const UserMap& fileInfo)
{
    for (const auto& i : fileInfo)
    {
        if (IN_CLOSE_WRITE != i.second)
        {
            continue;
        }

        collectRamoopsData();
    }
}

} // namespace ramoops
} // namespace dump
} // namespace phosphor
