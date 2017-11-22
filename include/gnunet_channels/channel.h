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
    using OnWrite   = std::function<void(sys::error_code, size_t)>;

public:
    Channel(Service&);
    Channel(std::shared_ptr<Cadet>);
    Channel(std::shared_ptr<ChannelImpl>);

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    Channel(Channel&&);
    Channel& operator=(Channel&&);

    asio::io_service& get_io_service();

    template<class Token>
    void
    connect( std::string target_id
           , const std::string& shared_secret
           , Token&&);

    template< class MutableBufferSequence
            , class ReadHandler>
    void async_read_some(const MutableBufferSequence&, ReadHandler&&);

    template< class ConstBufferSequence
            , class WriteHandler>
    void async_write_some(const ConstBufferSequence&, WriteHandler&&);

    ~Channel();

private:
    friend class ::gnunet_channels::CadetPort;

    void connect_impl( std::string target_id
                     , const std::string& shared_secret
                     , OnConnect);

    void receive_impl(std::vector<asio::mutable_buffer>, OnReceive);
    void write_impl(std::vector<uint8_t>, OnWrite);

    ChannelImpl* get_impl() { return _impl.get(); }
    void set_impl(std::shared_ptr<ChannelImpl>);

private:
    Scheduler& _scheduler;
    asio::io_service& _ios;
    std::shared_ptr<ChannelImpl> _impl;
};

//--------------------------------------------------------------------
template<class Token>
void
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

    result.get();
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

template< class ConstBufferSequence
        , class WriteHandler>
void Channel::async_write_some( const ConstBufferSequence& bufs
                              , WriteHandler&& h)
{
    using namespace std;

    // TODO: We could avoid one buffer copy if we put the data directly
    // into GNUNET_MQ_Envelope here.
    vector<uint8_t> data(asio::buffer_size(bufs));
    asio::buffer_copy(asio::buffer(data), bufs);

    write_impl(move(data), forward<WriteHandler>(h));
}

} // gnunet_channels namespace
