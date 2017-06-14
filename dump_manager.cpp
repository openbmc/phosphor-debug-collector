#include <sdbusplus/vtable.hpp>
#include "dump_manager.hpp"
#include "dump_create.hpp"
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include "xyz/openbmc_project/Common/error.hpp"
#include "config.h"
#include <unistd.h>
#include <sys/wait.h>

namespace phosphor
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;
namespace fs = std::experimental::filesystem;
using dumpType = 
       sdbusplus::xyz::openbmc_project::Dump::Internal::server::Create::Type;

namespace dump
{ 

namespace internal
{

void Manager::create(
    Type type,
    std::vector<std::string> fullPaths)
{
    // TODO openbmc/openbmc#1795 
    // Add implementaion of internal create function.
}

} //namepsace internal

uint32_t Manager::createDump()
{
    std::vector<std::string> paths;  

    auto dumpId = captureDump( 
            dumpType::UserRequested,paths)  ;

    return dumpId;
}

uint32_t Manager:: captureDump(
            dumpType type,
            std::vector<std::string> fullPaths)
{
    pid_t pid = fork();
    int status = 0;
    
    // TODO openbmc/openbmc#1795
    // Add Dump location info. 
    if (pid == 0)
    {
       execl("/usr/bin/ffdc","ffdc",(char*)0 );
       log<level::ERR>("Error occurred during ffdc function execution");
       //elog<InternalFailure>(); 
    }
    else if (pid > 0)
    {
        waitpid(pid, &status, 0);
    }
   
    else
    {
        log<level::ERR>("Error occurred during fork");
        //elog<InternalFailure>();
    }
    
    return ++entryId;
}

void Manager::createEntry(fs::path fullPath)
{
    // TODO openbmc/openbmc#1795
    // Get Dump ID from Dump file name. 
    // Validate the Dump file name. 
    auto id = entryId;

    //Get Epoch time.
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

    // Entry Object path.    
    auto objPath =  std::string(OBJ_ENTRY) + '/' +
                          std::to_string(id);

    auto dumpsize = fs::file_size(fullPath) ;

   // phosphor::dump::Entry i(busLog,objPath,ms,dumpsize);
    entries.insert(std::make_pair(entryId, 
            std::make_unique<Entry>(
            busLog,
            objPath.c_str(),
            id,
            ms,
            dumpsize,
            fullPath,
            *this)));
   return; 
}

void Manager::erase(uint32_t entryId)
{
    auto entry = entries.find(entryId);
    if(entries.end() != entry)
    {
        entries.erase(entry);
    }
}

} //namespace dump
} //namespace phosphor
