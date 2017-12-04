#pragma once

#include <gnunet_channels/channel.h>

struct GNUNET_CADET_Channel;
struct GNUNET_PeerIdentity;

namespace gnunet_channels {

class Service;
class ChannelImpl;

class CadetPort {
    struct Impl;

public:
    CadetPort(Service&);
    CadetPort(std::shared_ptr<Cadet>);

    CadetPort(const CadetPort&)            = delete;
    CadetPort& operator=(const CadetPort&) = delete;

    /*
     * Destructor may be called only when no open() handler is pending.
     */
    ~CadetPort();

    Scheduler& scheduler();
    asio::io_service& get_io_service();

    /*
     * open() may be called only when the Port is closed, and no simultaneous open() handler is pending.
     */
    template<class Token>
    typename asio::async_result<typename asio::handler_type<Token, void(sys::error_code)>::type>::type
    open(const std::string& shared_secret, Token&& token);

    /*
     * close() may be called only when no open() handler is pending.
     */
    void close();

    /*
     * accept() may be called only when the Port is open.
     */
    template<class Token>
    typename asio::async_result<typename asio::handler_type<Token, void(sys::error_code)>::type>::type
    accept(Channel& channel, Token&& token);

    /*
     * Cancels all active accept() calls.
     */
    void cancel_accept();

private:
    void open_impl(const std::string& shared_secret, std::function<void(sys::error_code)> handler);

    void accept_impl(Channel& channel, std::function<void(sys::error_code)> handler);

private:
    std::unique_ptr<Impl> _impl;
};

//--------------------------------------------------------------------
template<class Token>
typename asio::async_result<typename asio::handler_type<Token, void(sys::error_code)>::type>::type
CadetPort::open(const std::string& shared_secret, Token&& token)
{
    using Handler = typename asio::handler_type<Token, void(sys::error_code)>::type;

    Handler handler(std::forward<Token>(token));
    asio::async_result<Handler> result(handler);

    open_impl(shared_secret, std::move(handler));

    return result.get();
}

template<class Token>
typename asio::async_result<typename asio::handler_type<Token, void(sys::error_code)>::type>::type
CadetPort::accept(Channel& channel, Token&& token)
{
    using Handler = typename asio::handler_type<Token, void(sys::error_code)>::type;

    Handler handler(std::forward<Token>(token));
    asio::async_result<Handler> result(handler);

    accept_impl(channel, std::move(handler));

    return result.get();
}

} // gnunet_channels namespace
