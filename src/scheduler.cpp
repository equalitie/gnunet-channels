#include <iostream>
#include <gnunet/platform.h>
#include <gnunet/gnunet_cadet_service.h>

#include <gnunet_channels/error.h>
#include "scheduler.h"

using namespace std;
using namespace gnunet_channels;

static void throw_error(const error::error_t& ec)
{
    throw sys::system_error(make_error_code(ec));
}

Scheduler::Scheduler(string config, asio::io_service& ios)
    : _ios(ios)
{
    if (pipe2(_pipes, O_NONBLOCK) != 0) {
        throw_error(error::cant_create_pipes);
    }

    _gnunet_thread = std::thread(
        [this, config = move(config)] {
            GNUNET_GETOPT_CommandLineOption options[] = {
                GNUNET_GETOPT_OPTION_END
            };

            const char* argv[] = { "gnunet-channels", "-c", config.c_str() };
            int argc = sizeof(argv) / sizeof(const char*);

            
            int ret = GNUNET_PROGRAM_run2(
                       argc, (char* const*) argv, "gnunet-channels",
                       gettext_noop("GNUnet channels"),
                       options,
                       Scheduler::program_run, this,
                       GNUNET_YES);

            assert(ret == GNUNET_OK);
        });
}

void Scheduler::program_run( void *cls
                           , char *const *args
                           , const char *cfgfile
                           , const GNUNET_CONFIGURATION_Handle *cfg)
{

    auto self = static_cast<Scheduler*>(cls);
    self->_cfg = cfg;
    GNUNET_SCHEDULER_run_with_optional_signals( GNUNET_NO
                                              , Scheduler::scheduler_run
                                              , cls);
}

void Scheduler::scheduler_run(void *cls)
{
    auto self = static_cast<Scheduler*>(cls);
    self->wait_for_job();
}

void Scheduler::wait_for_job()
{
    struct Inner {
        static void drain_pipe(int fd) {
            char buffer[256];
            for (;;) {
                ssize_t s = read(fd, buffer, sizeof(buffer));
                if (s == 0) return;
                if (s == -1) {
                    assert(errno == EAGAIN);
                    return;
                }
            }
        }

        static void call(void *cls)
        {
            auto self = static_cast<Scheduler*>(cls);
            drain_pipe(self->_pipes[0]);

            while(true) {
                queue<Handler> handlers;
                {
                    lock_guard<mutex> lock(self->_mutex);
                    handlers = move(self->_handlers);
                    if (handlers.empty()) break;
                }
                while (!handlers.empty()) {
                    auto h = move(handlers.front());
                    handlers.pop();
                    h(self->_cfg);
                    if (self->_shutdown) return;
                }
            }

            self->wait_for_job();
        }
    };

    GNUNET_DISK_FileHandle rfd{_pipes[0]};

    _pipe_task = GNUNET_SCHEDULER_add_read_file
                    ( GNUNET_TIME_UNIT_FOREVER_REL
                    , &rfd
                    , Inner::call
                    , this);
}

void Scheduler::post(Handler f)
{
    {
        lock_guard<mutex> lock(_mutex);
        _handlers.push([ w = asio::io_service::work(_ios)
                       , f = move(f)
                       ] (auto arg) { f(arg); });
    }
    static char b = 0;
    write(_pipes[1], &b, 1);
}

void Scheduler::post(function<void()> f)
{
    post([f = move(f)](auto) { f(); });
}

Scheduler::~Scheduler()
{
    post([this] { 
            _shutdown = true;
            GNUNET_SCHEDULER_shutdown();
        });

    _gnunet_thread.join();

    close(_pipes[0]);
    close(_pipes[1]);
}

asio::io_service& Scheduler::get_io_service()
{
    return _ios;
}
