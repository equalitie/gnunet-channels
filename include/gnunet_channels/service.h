#pragma once

#include <boost/asio/io_service.hpp>
#include <gnunet_channels/namespaces.h>

namespace gnunet_channels {

class Cadet;

class Service {
    class Impl;
    using OnSetup = std::function<void(sys::error_code)>;

public:
    Service(std::string config_path, asio::io_service&);

    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;

    template<class Token>
    void async_setup(Token&& token);

    asio::io_service& get_io_service();

    ~Service();

    // TODO: This should be private.
    std::shared_ptr<Cadet>& cadet();

private:
    void async_setup_impl(OnSetup);

private:
    std::shared_ptr<Impl> _impl;
};

//--------------------------------------------------------------------
template<class Token>
void Service::async_setup(Token&& token)
{
    using Handler = typename asio::handler_type< Token
                                               , void(sys::error_code)
                                               >::type;

    Handler handler(std::forward<Token>(token));
    asio::async_result<Handler> result(handler);
    async_setup_impl(std::move(handler));
    result.get();
}

} // gnunet_channels namespace
