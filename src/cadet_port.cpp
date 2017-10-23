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
    bool was_destroyed = false;

    Impl(shared_ptr<Cadet> cadet)
        : cadet(move(cadet)) {}

    asio::io_service& get_io_service() {
        return cadet->get_io_service();
    }

    auto accept_fail(sys::error_code ec) {
        get_io_service().post([ ec
                              , c = cadet
                              , f = move(on_accept)] { f(ec, Channel(c)); });
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
    auto impl = static_cast<Impl*>(cls);

    auto ch = make_shared<ChannelImpl>(impl->cadet);

    ch->_channel = handle;

    impl->get_io_service().post([ch, impl = impl->shared_from_this()] {
            if (!impl->on_accept) {
                return ch->close();
            }

            if (impl->was_destroyed) {
                ch->close();
                return impl->accept_fail(asio::error::operation_aborted);
            }

            auto f = move(impl->on_accept);
            f(sys::error_code(), move(ch));
        });

    return ch.get();
}

void CadetPort::open(const string& shared_secret, OnAccept on_accept)
{
    GNUNET_HashCode port_hash;
    GNUNET_CRYPTO_hash(shared_secret.c_str(), shared_secret.size(), &port_hash);

    if (_impl->on_accept) {
        _impl->accept_fail(asio::error::operation_aborted);
    }

    _impl->on_accept = move(on_accept);

    scheduler().post([impl = _impl, port_hash] {
            if (impl->was_destroyed) {
                return impl->accept_fail(asio::error::operation_aborted);
            }

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

    scheduler().post([impl = move(_impl)] {
        GNUNET_CADET_close_port(impl->port);
        impl->port = nullptr;
    });
}
