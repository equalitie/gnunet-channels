#pragma once

#include <thread>
#include <mutex>
#include <queue>
#include <boost/asio/io_service.hpp>

#include <gnunet_channels/namespaces.h>

struct GNUNET_CONFIGURATION_Handle;
struct GNUNET_SCHEDULER_Task;

namespace gnunet_channels {

class Scheduler {
    using Handler = std::function<void(const GNUNET_CONFIGURATION_Handle*)>;

public:
    Scheduler(std::string config, asio::io_service&);

    // Send a task of type void() or void(const GNUNET_CONFIGURATION_Handle*)
    // to be executed in GNUnet's thread.
    void post(Handler);
    void post(std::function<void()>);

    asio::io_service& get_io_service();

    ~Scheduler();

private:
    static void program_run( void *cls
                           , char *const *args
                           , const char *cfgfile
                           , const GNUNET_CONFIGURATION_Handle *cfg);

    static void scheduler_run(void*);

    void wait_for_job();

    static void shutdown_task(void*);

private:
    asio::io_service& _ios;
    std::thread _gnunet_thread;
    int _pipes[2];
    bool _shutdown = false;
    const GNUNET_CONFIGURATION_Handle* _cfg = nullptr;
    GNUNET_SCHEDULER_Task* _pipe_task = nullptr;
    std::mutex _mutex;
    std::queue<Handler> _handlers;
};

} // gnunet_channels namespace
