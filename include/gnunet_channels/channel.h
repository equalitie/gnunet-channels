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

    void connect( std::string target_id
                , const std::string& shared_secret
                , OnConnect);

    void send(const std::string&);
    void receive(OnReceive);

    ~Channel();

private:
    Scheduler& _scheduler;
    asio::io_service& _ios;
    std::shared_ptr<ChannelImpl> _impl;
};

} // gnunet_channels namespace
