#include <gnunet_channels/service.h>
#include "scheduler.h"
#include "cadet_connect.h"
#include "hello_get.h"

using namespace std;
using namespace gnunet_channels;

struct Service::Impl {
    Impl(string config_path, asio::io_service& ios)
        : scheduler(move(config_path), ios)
    {}

    bool was_destroyed = false;

    Scheduler                     scheduler;
    std::shared_ptr<CadetConnect> cadet_connect;
    std::shared_ptr<Cadet>        cadet;
    std::shared_ptr<HelloGet>     hello_get;
    // TODO: This is currently unused, but may come in handy in the future.
    GNUNET_PeerIdentity           identity;
};

Service::Service(string config_path, asio::io_service& ios)
    : _impl(make_shared<Impl>(config_path, ios))
{
}

asio::io_service& Service::get_io_service()
{
    return _impl->scheduler.get_io_service();
}

shared_ptr<Cadet>& Service::cadet()
{
    return _impl->cadet;
}

void Service::async_setup_impl(OnSetup on_setup)
{
    // TODO: Return error code
    assert(!_impl->cadet_connect);

    _impl->cadet_connect = make_shared<CadetConnect>(_impl->scheduler);

    // TODO: Timeout
    _impl->cadet_connect->run([ impl     = _impl
                              , on_setup = move(on_setup)
                              ] (shared_ptr<Cadet> cadet) {
            if (impl->was_destroyed) return;

            impl->cadet = move(cadet);

            impl->hello_get = make_shared<HelloGet>(impl->scheduler);
            impl->hello_get->run([ impl     = move(impl)
                                 , on_setup = move(on_setup)] (HelloMessage m) {
                    if (impl->was_destroyed) return;

                    impl->identity = m.peer_identity();
                    on_setup(sys::error_code());
                });
        });
}

Service::~Service()
{
    _impl->was_destroyed = true;
}
