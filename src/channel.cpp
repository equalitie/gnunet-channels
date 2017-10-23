#include <gnunet_channels/channel.h>
#include "channel_impl.h"

using namespace std;
using namespace gnunet_channels;

Channel::Channel(std::shared_ptr<Cadet> cadet)
    : _scheduler(cadet->scheduler())
    , _ios(cadet->get_io_service())
    , _impl(make_shared<ChannelImpl>(move(cadet)))
{
}

Channel::Channel(std::shared_ptr<ChannelImpl> impl)
    : _scheduler(impl->scheduler())
    , _ios(impl->get_io_service())
    , _impl(move(impl))
{
}

asio::io_service& Channel::get_io_service()
{
    return _ios;
}

void Channel::connect( std::string target_id
                     , const std::string& shared_secret
                     , OnConnect h)
{
    _impl->connect(move(target_id), shared_secret, move(h));
}

void Channel::send(const std::string& data)
{
    _impl->send(data);
}

void Channel::receive(OnReceive h)
{
    _impl->receive(move(h));
}

Channel::~Channel()
{
    // Could have been moved from.
    if (_impl) _impl->close();
}
