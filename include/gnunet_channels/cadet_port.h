#pragma once

#include <gnunet_channels/channel.h>

namespace gnunet_channels {

class Service;
class ChannelImpl;

class CadetPort {
    struct Impl;

public:
    using OnAccept = std::function<void(sys::error_code)>;

public:
    CadetPort(Service&);
    CadetPort(std::shared_ptr<Cadet>);

    CadetPort(const CadetPort&)            = delete;
    CadetPort& operator=(const CadetPort&) = delete;

    template<class Token>
    typename asio::async_result
        < typename asio::handler_type<Token, void(sys::error_code)>::type
        >::type
    open(Channel&, const std::string& shared_secret, Token&&);

    Scheduler& scheduler();
    asio::io_service& get_io_service() { return _ios; }

    ~CadetPort();

private:
    void open_impl(Channel&, const std::string& shared_secret, OnAccept);

    static
    void* channel_incoming( void *cls
                          , GNUNET_CADET_Channel *handle
                          , const GNUNET_PeerIdentity *initiator);

    static
    void connect_channel_ended( void *cls
                              , const GNUNET_CADET_Channel *channel);

private:
    asio::io_service& _ios;
    std::shared_ptr<Impl> _impl;
};

//--------------------------------------------------------------------
template<class Token>
typename asio::async_result
    < typename asio::handler_type<Token, void(sys::error_code)>::type
    >::type
CadetPort::open(Channel& ch, const std::string& shared_secret, Token&& token)
{
    using Handler = typename asio::handler_type< Token
                                               , void(sys::error_code)
                                               >::type;

    Handler handler(std::forward<Token>(token));
    asio::async_result<Handler> result(handler);

    open_impl(ch, shared_secret, std::move(handler));

    return result.get();
}

} // gnunet_channels namespace
