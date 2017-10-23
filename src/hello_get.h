#pragma once

#include <iostream>
#include <functional>
#include <memory>

#include <gnunet/gnunet_transport_hello_service.h>

#include <hello_message.h>
#include <scheduler.h>

namespace gnunet_channels {

class HelloGet : public std::enable_shared_from_this<HelloGet> {
    using Handler = std::function<void(HelloMessage)>;

public:
    HelloGet(Scheduler&);

    HelloGet(const HelloGet&) = delete;
    HelloGet& operator=(const HelloGet&) = delete;

    void run(Handler);

private:
    static void callback(void*, const GNUNET_MessageHeader*);

private:
    Scheduler& _scheduler;
};

} // gnunet_channels namespace
