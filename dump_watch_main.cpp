#include <exception>
#include "dump_watch.hpp"

int main(int argc, char* argv[])
{
    sd_event* loop = nullptr;
    sd_event_default(&loop);

    try
    {
        phosphor::dump::Watch watch(loop);
        sd_event_loop(loop);
    }
    catch (std::exception& e)
    {
        //TODO Handle exception.
        return -1;
    }

    sd_event_unref(loop);

    return 0;
}
