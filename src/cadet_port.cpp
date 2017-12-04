#include <gnunet/platform.h>
#include <cassert>
#include "channel_impl.h"
#include <gnunet_channels/cadet_port.h>
#include <gnunet_channels/channel.h>
#include <gnunet_channels/error.h>
#include <gnunet_channels/service.h>

using namespace std;
using namespace gnunet_channels;

/*
 * Accessed only from the gnunet thread.
 */
struct OpenPort {
    asio::io_service& ios;
    const std::shared_ptr<Cadet> cadet;
    GNUNET_CADET_Port* port;
    std::queue<std::shared_ptr<ChannelImpl>> channel_queue;
    std::queue<std::function<void(std::shared_ptr<ChannelImpl>)>> waiter_queue;

    OpenPort(asio::io_service& ios_, const std::shared_ptr<Cadet>& cadet_)
        : ios(ios_)
        , cadet(cadet_)
        , port(nullptr)
    {}
};

struct CadetPort::Impl {
    asio::io_service& ios;
    const std::shared_ptr<Cadet> cadet;
    std::unique_ptr<OpenPort> port;

    Impl(std::shared_ptr<Cadet> cadet_)
        : ios(cadet_->get_io_service())
        , cadet(std::move(cadet_))
    {}
};

CadetPort::CadetPort(Service& service)
    : CadetPort(service.cadet())
{}

CadetPort::CadetPort(std::shared_ptr<Cadet> cadet)
    : _impl(std::make_unique<Impl>(cadet))
{
}

CadetPort::~CadetPort()
{
    close();
}

Scheduler& CadetPort::scheduler()
{
    return _impl->cadet->scheduler();
}

asio::io_service& CadetPort::get_io_service()
{
    return _impl->ios;
}

// Executed in GNUnet's thread
static void* channel_incoming(void* cls, GNUNET_CADET_Channel* channel_handle, const GNUNET_PeerIdentity* initiator)
{
    OpenPort* port = static_cast<OpenPort*>(cls);
    std::shared_ptr<ChannelImpl> channel_impl = std::make_shared<ChannelImpl>(port->cadet);
    channel_impl->set_handle(channel_handle);

    if (port->waiter_queue.empty()) {
        std::function<void(std::shared_ptr<ChannelImpl>)> handler = port->waiter_queue.front();
        port->waiter_queue.pop();
        port->ios.post([handler = std::move(handler), channel_impl] {
            handler(channel_impl);
        });
    } else {
        port->channel_queue.push(channel_impl);
    }

    return channel_impl.get();
}

void CadetPort::open_impl(const std::string& shared_secret, std::function<void(sys::error_code)> handler)
{
    if (_impl->port) {
        assert(false);
    }

    _impl->port = std::make_unique<OpenPort>(_impl->ios, _impl->cadet);

    GNUNET_HashCode port_hash;
    GNUNET_CRYPTO_hash(shared_secret.c_str(), shared_secret.size(), &port_hash);

    scheduler().post([
        this,
        port_hash,
        handler = std::move(handler)
    ] () mutable {
        GNUNET_MQ_MessageHandler handlers[] = {
            GNUNET_MQ_MessageHandler{
                ChannelImpl::check_data,
                ChannelImpl::handle_data,
                NULL,
                GNUNET_MESSAGE_TYPE_CADET_CLI,
                sizeof(GNUNET_MessageHeader)
            },
            GNUNET_MQ_handler_end()
        };

        _impl->port->port = GNUNET_CADET_open_port(
            _impl->cadet->handle(),
            &port_hash,
            channel_incoming,
            _impl->port.get(),
            NULL,
            ChannelImpl::connect_channel_ended,
            handlers
        );

        if (_impl->port->port) {
            _impl->port->ios.post([handler = std::move(handler)] {
                handler(sys::error_code());
            });
        } else {
            _impl->port->ios.post([
                handler = std::move(handler),
                this
            ] () mutable {
                _impl->port = nullptr;
                handler(error::failed_to_open_port);
            });
        }
    });
}

void CadetPort::close()
{
    if (!_impl->port) {
        return;
    }

    if (_impl->port && !_impl->port->port) {
        assert(false);
    }

    std::unique_ptr<OpenPort> port = std::move(_impl->port);
    _impl->port = nullptr;

    // Is there a better way to avoid this std::unique_ptr mess?
    scheduler().post([port_ = port.release()] {
        std::unique_ptr<OpenPort> port(port_);
        GNUNET_CADET_close_port(port->port);
        while (!port->channel_queue.empty()) {
            auto channel = std::move(port->channel_queue.front());
            port->channel_queue.pop();
            channel->close();
        }
        while (!port->waiter_queue.empty()) {
            auto waiter = std::move(port->waiter_queue.front());
            port->waiter_queue.pop();
            port->ios.post([waiter = std::move(waiter)] {
                waiter(nullptr);
            });
        }
    });
}

void CadetPort::accept_impl(Channel& channel, std::function<void(sys::error_code)> handler)
{
    if (!_impl->port) {
        assert(false);
    }

    auto submit_channel = [
        &channel,
        handler = std::move(handler)
    ] (std::shared_ptr<ChannelImpl> channel_impl) {
        if (channel_impl) {
            channel = Channel(channel_impl);
            handler(sys::error_code());
        } else {
            handler(sys::errc::make_error_code(sys::errc::operation_canceled));
        }
    };

    scheduler().post([
        submit_channel = std::move(submit_channel),
        port = _impl->port.get()
    ] {
        if (port->channel_queue.empty()) {
            port->waiter_queue.push(std::move(submit_channel));
        } else {
            std::shared_ptr<ChannelImpl> channel_impl = port->channel_queue.front();
            port->channel_queue.pop();
            port->ios.post([
                submit_channel = std::move(submit_channel),
                channel_impl
            ] {
                submit_channel(channel_impl);
            });
        }
    });
}

void CadetPort::cancel_accept()
{
    if (!_impl->port) {
        return;
    }

    scheduler().post([port = _impl->port.get()] {
        while (!port->waiter_queue.empty()) {
            auto waiter = port->waiter_queue.front();
            port->waiter_queue.pop();
            port->ios.post([waiter = std::move(waiter)] {
                waiter(nullptr);
            });
        }
    });
}
