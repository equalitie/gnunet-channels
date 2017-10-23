#pragma once

#include <gnunet/platform.h>
#include <memory>
#include "scheduler.h"
#include <gnunet/gnunet_cadet_service.h>

namespace gnunet_channels {

class Cadet : public std::enable_shared_from_this<Cadet> {
public:
    Cadet(Scheduler&, GNUNET_CADET_Handle*);

    Cadet(const Cadet&) = delete;
    Cadet& operator=(const Cadet&) = delete;

    Scheduler&           scheduler()      { return _scheduler; }
    GNUNET_CADET_Handle* handle()         { return _handle; }
    asio::io_service&    get_io_service() { return _scheduler.get_io_service(); }

    ~Cadet();

private:
    Scheduler& _scheduler;
    GNUNET_CADET_Handle* _handle;
};

} // gnunet_channels
