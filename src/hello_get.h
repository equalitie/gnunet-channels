#pragma once

#include <iostream>
#include <functional>
#include <memory>

#include <gnunet/gnunet_transport_hello_service.h>

#include "hello_message.h"
#include "scheduler.h"

namespace gnunet_channels {

class HelloGet : public std::enable_shared_from_this<HelloGet> {
    class Task;
    using Handler = std::function<void(HelloMessage)>;

public:
    HelloGet(Scheduler&);

    HelloGet(const HelloGet&) = delete;
    HelloGet& operator=(const HelloGet&) = delete;

    void run(Handler);

    asio::io_service& get_io_service();

    void close();

private:
    static void callback(void*, const GNUNET_MessageHeader*);

private:
    Scheduler& _scheduler;
    Task* _task = nullptr;
};

} // gnunet_channels namespace
