#include <gnunet/platform.h>
#include "cadet_port.h"
#include "channel_impl.h"
#include <iostream>
#include <gnunet_channels/channel.h>
#include <gnunet_channels/error.h>

using namespace std;
using namespace gnunet_channels;

CadetPort::CadetPort(shared_ptr<Cadet> cadet)
    : _ios(cadet->get_io_service())
    , _cadet(move(cadet))
{}

Scheduler& CadetPort::scheduler()
{
    return _cadet->scheduler();
}

// Executed in GNUnet's thread
void* CadetPort::channel_incoming( void *cls
                                 , GNUNET_CADET_Channel *handle
                                 , const GNUNET_PeerIdentity *initiator)
{
    auto port = static_cast<CadetPort*>(cls);

    auto ch = make_shared<ChannelImpl>(port->_cadet);

    ch->_channel = handle;

    port->get_io_service().post([ch, port = port->shared_from_this()] {
            if (!port->_on_accept) {
                return ch->close();
            }
            // First move the _on_accept lambda away because executing it might
            // destroy it while it's running.
            auto f = move(port->_on_accept);
            f(sys::error_code(), move(ch));
        });

    return ch.get();
}

void CadetPort::open(const string& shared_secret, OnAccept on_accept)
{
    GNUNET_HashCode port_hash;
    GNUNET_CRYPTO_hash(shared_secret.c_str(), shared_secret.size(), &port_hash);

    _on_accept = move(on_accept);

    scheduler().post([ this
                     , self = shared_from_this()
                     , port_hash
                     ] {
            GNUNET_MQ_MessageHandler handlers[] = {
                GNUNET_MQ_MessageHandler{ ChannelImpl::check_data
                                        , ChannelImpl::handle_data
                                        , NULL
                                        , GNUNET_MESSAGE_TYPE_CADET_CLI
                                        , sizeof(GNUNET_MessageHeader) },
                GNUNET_MQ_handler_end ()
            };

            _port = GNUNET_CADET_open_port( _cadet->handle()
                                          , &port_hash
                                          , CadetPort::channel_incoming
                                          , self.get()
                                          , NULL
                                          , ChannelImpl::connect_channel_ended
                                          , handlers);

            if (!_port) {
                self->get_io_service().post([self] {
                        auto f = move(self->_on_accept);
                        f(error::failed_to_open_port, Channel(self->_cadet));
                    });
            }
        });
}

void CadetPort::close()
{
    if (!_port) return;
    scheduler().post([c = _cadet, p = _port] {
        GNUNET_CADET_close_port(p);
    });
    _port = nullptr;
}

CadetPort::~CadetPort()
{
    close();
}
