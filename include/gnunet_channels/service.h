#pragma once

#include <boost/asio/io_service.hpp>
#include <gnunet_channels/namespaces.h>

namespace gnunet_channels {

class Cadet;

class Service {
    class Impl;
    using OnSetup = std::function<void(sys::error_code)>;

public:
    Service(std::string config_path, asio::io_service& ios);

    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;

    void async_setup(OnSetup);

    asio::io_service& get_io_service();

    ~Service();

    // TODO: This should be private.
    std::shared_ptr<Cadet>& cadet();

private:
    std::shared_ptr<Impl> _impl;
};

} // gnunet_channels namespace
