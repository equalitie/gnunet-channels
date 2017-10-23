#pragma once

#include "cadet.h"
namespace gnunet_channels {

class CadetConnect : public std::enable_shared_from_this<CadetConnect> {
public:
    CadetConnect(Scheduler&);

    template<class Handler> void run(Handler h);

private:
    Scheduler& _scheduler;
};

inline
CadetConnect::CadetConnect(Scheduler& scheduler)
    : _scheduler(scheduler)
{
}

template<class Handler>
inline void CadetConnect::run(Handler h)
{
    using std::move;
    using std::make_shared;

    auto s = shared_from_this();

    _scheduler.post([this, s = move(s), h = move(h)]
                    (const GNUNET_CONFIGURATION_Handle* cfg) {
            GNUNET_CADET_Handle *handle = GNUNET_CADET_connect(cfg);

            auto& ios = _scheduler.get_io_service();

            ios.post([this, s = move(s), h = move(h), handle] {
                         h(make_shared<Cadet>(_scheduler, handle));
                     });
        });
}

} // gnunet_channels namespace
