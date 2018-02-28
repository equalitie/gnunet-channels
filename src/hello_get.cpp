#include <gnunet/platform.h>
#include "hello_get.h"

using namespace std;
using namespace gnunet_channels;

HelloGet::HelloGet(Scheduler& scheduler)
    : _scheduler(scheduler)
{ }

asio::io_service& HelloGet::get_io_service()
{
    return _scheduler.get_io_service();
}

struct HelloGet::Task {
    asio::io_service::work work;
    Handler h;
    shared_ptr<HelloGet> s;
    GNUNET_TRANSPORT_HelloGetHandle* get_handle = nullptr;

    static void call(void* ctx, const GNUNET_MessageHeader* hello)
    {
        auto s = static_cast<Task*>(ctx)->s;
        auto& ios = s->get_io_service();

        // If get_handle is null, HelloGet::close was called.
        if (!s->_task->get_handle) return;

        GNUNET_TRANSPORT_hello_get_cancel(s->_task->get_handle);
        s->_task->get_handle = nullptr;

        auto m = (GNUNET_HELLO_Message*) GNUNET_copy_message(hello);

        ios.post([s, m = move(m)]() mutable {
                auto h = move(s->_task->h);
                delete s->_task;
                s->_task = nullptr;
                h(HelloMessage(m));
            });
    }
};

void HelloGet::close()
{
    auto &ios = get_io_service();

    if (!_task) return;

    _scheduler.post([ &ios
                    , s = shared_from_this() ] {
            if (!s->_task->get_handle) return;

            GNUNET_TRANSPORT_hello_get_cancel(s->_task->get_handle);
            s->_task->get_handle = nullptr;

            ios.post([s] {
                    auto h = move(s->_task->h);
                    delete s->_task;
                    s->_task = nullptr;
                    h(HelloMessage(nullptr));
                });
        });
}

void HelloGet::run(Handler h)
{
    auto &ios = get_io_service();

    _task = new Task{ asio::io_service::work(get_io_service())
                    , move(h)
                    , shared_from_this() };

    _scheduler.post([ s = shared_from_this()
                    , h = move(h)
                    , &ios
                    ] (const GNUNET_CONFIGURATION_Handle *config) {
            s->_task->get_handle
                = GNUNET_TRANSPORT_hello_get( config
                                            , GNUNET_TRANSPORT_AC_ANY
                                            , Task::call
                                            , s->_task);
        });
}
