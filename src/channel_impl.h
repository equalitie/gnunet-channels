#pragma once

#include <gnunet/platform.h>
#include "cadet.h"
#include <gnunet_channels/namespaces.h>
#include <gnunet_channels/channel.h>

namespace gnunet_channels {

class ChannelImpl : public std::enable_shared_from_this<ChannelImpl> {
public:
    using OnConnect = std::function<void(sys::error_code)>;
    using OnReceive = std::function<void(sys::error_code, size_t)>;
    using OnSend    = std::function<void(sys::error_code, size_t)>;

private:
    struct Buffer {
        std::vector<uint8_t> data;
        asio::const_buffer   info;

        Buffer(std::vector<uint8_t> data, size_t start = 0)
            : data(std::move(data))
            , info(this->data.data(), this->data.size())
        {
            info = info + start;
        }
    };

    struct SendEntry {
        std::vector<uint8_t> data;
        OnSend on_send;
    };

public:
    ChannelImpl(std::shared_ptr<Cadet>);

    Scheduler& scheduler();
    asio::io_service& get_io_service();

    void connect( std::string target_id
                , const std::string& shared_secret
                , OnConnect);

    void send(std::vector<uint8_t>, OnSend);
    void receive(std::vector<asio::mutable_buffer>, OnReceive);
    void close();

    ~ChannelImpl();

private:
    friend class CadetPort;
    friend class ::gnunet_channels::Channel;

    static void  handle_data(void *cls, const GNUNET_MessageHeader*);
    static int   check_data(void *cls, const GNUNET_MessageHeader*);
    static void  connect_channel_ended(void *cls, const GNUNET_CADET_Channel*);
    static void  connect_window_change(void *cls, const GNUNET_CADET_Channel*, int);
    static void* channel_incoming(void *, GNUNET_CADET_Channel*, const GNUNET_PeerIdentity*);
    static void  data_sent(void *cls);

    void do_send(std::vector<uint8_t>, OnSend);
private:
    OnConnect _on_connect;
    OnReceive _on_receive;
    std::function<void(sys::error_code)> _on_send;

    // This one is mutable and can only be modified (and read) inside the
    // GNUnet's thread.
    GNUNET_CADET_Channel* _channel = nullptr;
    std::shared_ptr<Cadet> _cadet;
    Scheduler& _scheduler;

    std::queue<Buffer> _recv_queue;
    std::queue<SendEntry> _send_queue;
    std::vector<asio::mutable_buffer> _output;
};

} // gnunet_channels namespace
