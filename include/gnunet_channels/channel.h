#pragma once

#include <memory>
#include <boost/asio/io_service.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <gnunet_channels/namespaces.h>

struct GNUNET_CADET_Channel;

namespace gnunet_channels {

class Cadet;
class ChannelImpl;
class Scheduler;
class Service;
class CadetPort;

class Channel {
public:
    using OnConnect = std::function<void(sys::error_code)>;
    using OnReceive = std::function<void(sys::error_code, size_t)>;

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

    template< class MutableBufferSequence
            , class ReadHandler>
    void async_read_some(const MutableBufferSequence&, ReadHandler&&);

    ~Channel();

private:
    friend class ::gnunet_channels::CadetPort;

    void connect_impl( std::string target_id
                     , const std::string& shared_secret
                     , OnConnect);

    void receive_impl(std::vector<asio::mutable_buffer>, OnReceive);

    void set_handle(GNUNET_CADET_Channel*);
    ChannelImpl* get_impl() { return _impl.get(); }

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

template<class T> class Debug;

template< class MutableBufferSequence
        , class ReadHandler>
void Channel::async_read_some( const MutableBufferSequence& bufs
                             , ReadHandler&& h)
{
    using namespace std;

    // TODO: Small Object Optimization: I presume most of the times the size of
    // bufs is going to be rather small. So to avoid allocation of memory
    // inside std::vector we could instead store it in std::array. I.e. we
    // could store it in e.g. variant<vector, array> depending on the size.
    // TODO: Also, can we use std::size instead of std::distance?
    vector<asio::mutable_buffer> bs(distance(bufs.begin(), bufs.end()));

    copy(bufs.begin(), bufs.end(), bs.begin());

    receive_impl(move(bs), forward<ReadHandler>(h));
}

} // gnunet_channels namespace
