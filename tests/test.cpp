#define BOOST_TEST_MODULE GNUnet_Channels_Tests
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <chrono>
#include <thread>

#include <gnunet_channels/channel.h>
#include <gnunet_channels/cadet_port.h>
#include <gnunet_channels/service.h>
#include <gnunet_channels/namespaces.h>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>

#include <sys/wait.h>

using namespace std;
using namespace gnunet_channels;
using namespace chrono_literals;

static const string config1 = "../scripts/peer1.conf";
static const string config2 = "../scripts/peer2.conf";

//--------------------------------------------------------------------
// Unfortunately GNUnet won't let us run more than one node per
// one process, so we need to fork one new process per node and
// run test code from there.
class Fork {
private:
    using Func = function<void(Service& service, asio::yield_context yield)>;

public:
    Fork(string config, Func func)
    {
        pid = fork();
        BOOST_CHECK(pid >= 0);
        if (!is_child()) return;

        {
            asio::io_service ios;
            Service service(config, ios);

            asio::spawn(ios, [&] (auto yield) {
                    // TODO: We shouldn't need to instantiate work here.
                    asio::io_service::work w(ios);
                    sys::error_code ec;
                    service.async_setup(yield[ec]);

                    if (ec) {
                        cerr << "Failed to set up gnunet service: "
                             << ec.message() << endl;
                        return;
                    }

                    try {
                        func(service, yield);
                    }
                    catch (const exception& e) {
                        BOOST_ERROR("Exception was thrown");
                    }
                });

            ios.run();
        }
        exit(0);
    }

    ~Fork() {
        if (is_child()) return;
        int status;
        waitpid(pid, &status, 0);
    }

    bool is_child() const { return pid == 0; }

private:
    pid_t pid;
};

//--------------------------------------------------------------------
// If an instance of this class is not destroyed within a timeout
// then the test fails.
struct FailTimeout {
    using Duration = asio::steady_timer::duration;

    FailTimeout(Duration d, string task_name)
        : _task_name(move(task_name))
    {
        _keep_going.test_and_set();

        _thread = thread([this, d] {
                Duration delta = 200ms;
                Duration total = 0s;

                while (_keep_going.test_and_set()) {
                    this_thread::sleep_for(delta);
                    total += delta;

                    if (total > d) {
                        BOOST_ERROR("Task \""
                                   + _task_name
                                   + "\" takes too long");
                        exit(1);
                    }
                }
            });
    }

    ~FailTimeout()
    {
        _keep_going.clear();
        _thread.join();
    }

    thread _thread;
    atomic_flag _keep_going;
    string _task_name;
};

//--------------------------------------------------------------------
static string get_id(string config)
{
    FailTimeout ft(3s, "get_id");

    asio::io_service ios;
    Service service(config, ios);

    string result_id;

    asio::spawn(ios, [&] (auto yield) {
            service.async_setup(yield);
            result_id = service.identity();
        });

    ios.run();

    return result_id;
}

//--------------------------------------------------------------------
static string random_port()
{
    srand(time(0));
    stringstream ss;
    ss << "port_" << rand();
    return ss.str();
}

//--------------------------------------------------------------------
// Actual tests
//--------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(test_get_id_config1)
{
    string server_id = get_id(config1);
    BOOST_REQUIRE(!server_id.empty());
}

//--------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(test_get_id_config2)
{
    string server_id = get_id(config2);
    BOOST_REQUIRE(!server_id.empty());
}

//--------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(test)
{
    const string port = random_port();

    string server_id = get_id(config1);

    Fork n1(config1, [&](Service& service, auto yield) {
            FailTimeout ft(3s, "server");

            sys::error_code ec;
            Channel channel(service);
            CadetPort p(service);
            p.open(channel, port, yield);
        });

    Fork n2(config2, [&](Service& service, auto yield) {
            FailTimeout ft(4s, "client");

            sys::error_code ec;
            asio::steady_timer t(service.get_io_service());
            t.expires_from_now(1s);
            t.async_wait(yield[ec]);
            Channel channel(service);
            channel.connect(server_id, port, yield);
        });
}

//--------------------------------------------------------------------
