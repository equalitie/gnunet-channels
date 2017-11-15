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

void HelloGet::run(Handler h)
{
    _scheduler.post([ s = shared_from_this()
                    , h = move(h)
                    ] (const GNUNET_CONFIGURATION_Handle *config) {

            struct Task {
                asio::io_service::work work;
                Handler h;
                shared_ptr<HelloGet> s;
                GNUNET_TRANSPORT_HelloGetHandle *get = nullptr;

                static void call(void* ctx, const GNUNET_MessageHeader* hello)
                {
                    auto t = static_cast<Task*>(ctx);
                    auto& ios = t->s->get_io_service();

                    GNUNET_TRANSPORT_hello_get_cancel(t->get);
                    t->get = nullptr;

                    auto m = (GNUNET_HELLO_Message*) GNUNET_copy_message(hello);

                    ios.post([t, m = move(m)]() mutable {
                            auto h = move(t->h);
                            delete t;
                            h(HelloMessage(m));
                        });
                }
            };

            auto task = new Task{ asio::io_service::work(s->get_io_service())
                                , move(h)
                                , move(s) };

            auto get = GNUNET_TRANSPORT_hello_get( config
                                                 , GNUNET_TRANSPORT_AC_ANY
                                                 , Task::call
                                                 , task);
            task->get = get;
        });
}
