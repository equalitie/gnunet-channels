#include <gnunet/platform.h>
#include "channel_impl.h"
#include <iostream>
#include <gnunet_channels/cadet_port.h>
#include <gnunet_channels/channel.h>
#include <gnunet_channels/error.h>
#include <gnunet_channels/service.h>

using namespace std;
using namespace gnunet_channels;

struct CadetPort::Impl : public enable_shared_from_this<Impl> {
    shared_ptr<Cadet> cadet;
    GNUNET_CADET_Port *port = nullptr;
    OnAccept on_accept;
    ChannelImpl* channel = nullptr;
    bool was_destroyed = false;
    std::queue<shared_ptr<ChannelImpl>> queued_connections;

    Impl(shared_ptr<Cadet> cadet)
        : cadet(move(cadet)) {}

    asio::io_service& get_io_service() {
        return cadet->get_io_service();
    }

    auto accept_fail(sys::error_code ec) {
        get_io_service().post([ ec
                              , c = cadet
                              , f = move(on_accept)] { f(ec); });
    };
};

CadetPort::CadetPort(Service& service)
    : CadetPort(service.cadet())
{}

CadetPort::CadetPort(shared_ptr<Cadet> cadet)
    : _ios(cadet->get_io_service())
    , _impl(make_shared<Impl>(move(cadet)))
{}

Scheduler& CadetPort::scheduler()
{
    return _impl->cadet->scheduler();
}

// Executed in GNUnet's thread
void* CadetPort::channel_incoming( void *cls
                                 , GNUNET_CADET_Channel *handle
                                 , const GNUNET_PeerIdentity *initiator)
{
    auto port_impl = static_cast<Impl*>(cls);

    // NOTE: The pointer returned from this function will be used as a `cls` in
    // the ChannelImpl::connect_channel_ended callback.

    // TODO: Add a mutex around port_impl->{channel, cadet}
    shared_ptr<ChannelImpl> ret;
    bool queue_it = false;

    if (port_impl->channel) {
        ret = port_impl->channel->shared_from_this();
        port_impl->channel = nullptr;
    }
    else {
        ret = make_shared<ChannelImpl>(port_impl->cadet);
        queue_it = true;
    }

    ret->_handle = handle;

    port_impl->get_io_service().post(
        [ port_impl = port_impl->shared_from_this()
        , queue_it
        , ret
        ] {
            if (!port_impl->on_accept) {
                if (queue_it) {
                    port_impl->queued_connections.push(ret);
                }
                return;
            }

            if (port_impl->was_destroyed) {
                return port_impl->accept_fail(asio::error::operation_aborted);
            }

            if (queue_it) {
                // TODO: If the size of queued_connection is too big, destroy one.
                port_impl->queued_connections.push(ret);
            }
            else {
                auto f = move(port_impl->on_accept);
                f(sys::error_code());
            }
        });

    return ret.get();
}

void CadetPort::open_impl(Channel& ch, const string& shared_secret, OnAccept on_accept)
{
    GNUNET_HashCode port_hash;
    GNUNET_CRYPTO_hash(shared_secret.c_str(), shared_secret.size(), &port_hash);

    if (_impl->on_accept) {
        _impl->accept_fail(asio::error::operation_aborted);
    }

    if (!_impl->queued_connections.empty()) {
        auto ch_impl = move(_impl->queued_connections.front());
        _impl->queued_connections.pop();

        _ios.post([ ch_impl   = move(ch_impl)
                  , on_accept = move(on_accept)
                  , port_impl = _impl
                  , &ch
                  ] {
                if (port_impl->was_destroyed) {
                    ch_impl->close();
                    return on_accept(asio::error::operation_aborted);
                }

                ch.set_impl(move(ch_impl));
                on_accept(sys::error_code());
            });

        return;
    }

    _impl->channel = ch.get_impl();

    // TODO: Not sure how efficient it is to wrap the on_accept functor here.
    // We do need to create a io_service's work to prevent the main loop from
    // exiting once the lambda posted to the scheduler (below) is executed.
    // Perhaps on_accet could have a custom struct to hold OnAccept and the
    // work?
    _impl->on_accept = [ w         = asio::io_service::work(get_io_service())
                       , on_accept = move(on_accept)
                       ] (sys::error_code ec) { on_accept(ec); };

    scheduler().post([impl = _impl, port_hash] {
            if (impl->was_destroyed) {
                return impl->accept_fail(asio::error::operation_aborted);
            }

            if (impl->port) return;

            GNUNET_MQ_MessageHandler handlers[] = {
                GNUNET_MQ_MessageHandler{ ChannelImpl::check_data
                                        , ChannelImpl::handle_data
                                        , NULL
                                        , GNUNET_MESSAGE_TYPE_CADET_CLI
                                        , sizeof(GNUNET_MessageHeader) },
                GNUNET_MQ_handler_end()
            };

            impl->port = GNUNET_CADET_open_port( impl->cadet->handle()
                                               , &port_hash
                                               , CadetPort::channel_incoming
                                               , impl.get()
                                               , NULL
                                               , ChannelImpl::connect_channel_ended
                                               , handlers);

            if (!impl->port) {
                impl->accept_fail(error::failed_to_open_port);
            }
        });
}

CadetPort::~CadetPort()
{
    _impl->was_destroyed = true;

    // Need to get the scheduler here because the function internally uses
    // _impl which is moved from in the next step.
    auto& s = scheduler();

    s.post([impl = move(_impl)] {
        if (!impl->port) return;
        GNUNET_CADET_close_port(impl->port);
        impl->port = nullptr;
    });
}
