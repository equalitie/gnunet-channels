#include "cadet.h"

using namespace gnunet_channels;

Cadet::Cadet(Scheduler& scheduler, GNUNET_CADET_Handle* handle)
    : _scheduler(scheduler)
    , _handle(handle)
{}

Cadet::~Cadet()
{
    if (_handle) {
        _scheduler.post([h = _handle](auto) { GNUNET_CADET_disconnect(h); });
        _handle = nullptr;
    }
}

