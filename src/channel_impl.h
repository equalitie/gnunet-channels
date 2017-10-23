#pragma once

#include <gnunet/platform.h>
#include "cadet.h"
#include <gnunet_channels/namespaces.h>

namespace gnunet_channels {

class ChannelImpl : public std::enable_shared_from_this<ChannelImpl> {
public:
    using OnConnect = std::function<void(sys::error_code)>;
    using OnReceive = std::function<void(sys::error_code, std::string)>;

public:
    ChannelImpl(std::shared_ptr<Cadet>);

    Scheduler& scheduler();
    asio::io_service& get_io_service();

    void connect( std::string target_id
                , const std::string& shared_secret
                , OnConnect);

    void send(const std::string&);
    void receive(OnReceive);
    void close();

    ~ChannelImpl();

private:
    friend class CadetPort;

    static void  handle_data(void *cls, const GNUNET_MessageHeader*);
    static int   check_data(void *cls, const GNUNET_MessageHeader*);
    static void  connect_channel_ended(void *cls, const GNUNET_CADET_Channel*);
    static void  connect_window_change(void *cls, const GNUNET_CADET_Channel*, int);
    static void* channel_incoming(void *, GNUNET_CADET_Channel*, const GNUNET_PeerIdentity*);

private:
    OnConnect _on_connect;
    OnReceive _on_receive;

    // What is inside this structure can only be accessed
    // from the GNUnet thread.

    // This one is mutable and can only be modified (and read) inside the
    // GNUnet's thread.
    GNUNET_CADET_Channel* _channel = nullptr;
    std::shared_ptr<Cadet> _cadet;
    Scheduler& _scheduler;

    std::queue<std::string> _recv_queue;
};

} // gnunet_channels namespace
