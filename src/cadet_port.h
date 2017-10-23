#pragma once

#include <gnunet_channels/channel.h>
#include "cadet.h"

namespace gnunet_channels {

class Service;
class ChannelImpl;

class CadetPort : public std::enable_shared_from_this<CadetPort> {
public:
    using OnAccept = std::function<void(sys::error_code, Channel)>;

public:
    CadetPort(Service&);
    CadetPort(std::shared_ptr<Cadet>);

    void open(const std::string& shared_secret, OnAccept);
    void close();

    Scheduler& scheduler();
    asio::io_service& get_io_service() { return _ios; }

    ~CadetPort();

private:
    static
    void* channel_incoming( void *cls
                          , GNUNET_CADET_Channel *handle
                          , const GNUNET_PeerIdentity *initiator);

    static
    void connect_channel_ended( void *cls
                              , const GNUNET_CADET_Channel *channel);

private:
    asio::io_service& _ios;
    std::shared_ptr<Cadet> _cadet;
    GNUNET_CADET_Port *_port = nullptr;
    OnAccept _on_accept;
};

} // gnunet_channels namespace
