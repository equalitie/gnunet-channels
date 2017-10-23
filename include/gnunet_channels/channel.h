#pragma once

#include <memory>
#include <boost/asio/io_service.hpp>
#include <gnunet_channels/namespaces.h>

namespace gnunet_channels {

class Cadet;
class ChannelImpl;
class Scheduler;
class Service;

class Channel {
public:
    using OnConnect = std::function<void(sys::error_code)>;
    using OnReceive = std::function<void(sys::error_code, std::string)>;

public:
    Channel(Service&);
    Channel(std::shared_ptr<Cadet>);
    Channel(std::shared_ptr<ChannelImpl>);

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    Channel(Channel&&) = default;
    Channel& operator=(Channel&&) = default;

    asio::io_service& get_io_service();

    template<class Token>
    typename asio::async_result
        < typename asio::handler_type<Token, void(sys::error_code)>::type
        >::type
    connect( std::string target_id
           , const std::string& shared_secret
           , Token&&);

    void send(const std::string&);

    template<class Token>
    typename asio::async_result
        < typename asio::handler_type<Token, void(sys::error_code, std::string)>::type
        >::type
    receive(Token&& token);

    ~Channel();

private:
    void connect_impl( std::string target_id
                     , const std::string& shared_secret
                     , OnConnect);

    void receive_impl(OnReceive);

private:
    Scheduler& _scheduler;
    asio::io_service& _ios;
    std::shared_ptr<ChannelImpl> _impl;
};

//--------------------------------------------------------------------
template<class Token>
typename asio::async_result
    < typename asio::handler_type<Token, void(sys::error_code)>::type
    >::type
Channel::connect( std::string target_id
                , const std::string& shared_secret
                , Token&& token)
{
    using Handler = typename asio::handler_type< Token
                                               , void(sys::error_code)
                                               >::type;

    Handler handler(std::forward<Token>(token));
    asio::async_result<Handler> result(handler);

    connect_impl(std::move(target_id), shared_secret, std::move(handler));

    return result.get();
}

template<class Token>
typename asio::async_result
    < typename asio::handler_type<Token, void(sys::error_code, std::string)>::type
    >::type
Channel::receive(Token&& token)
{
    using Handler = typename asio::handler_type< Token
                                               , void(sys::error_code, std::string)
                                               >::type;

    Handler handler(std::forward<Token>(token));
    asio::async_result<Handler> result(handler);

    receive_impl(std::move(handler));

    return result.get();
}

} // gnunet_channels namespace
