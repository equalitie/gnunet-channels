#include <iostream>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/read_until.hpp>

#include <gnunet/platform.h>
#include <gnunet/gnunet_cadet_service.h>

#include <scheduler.h>
#include <gnunet_channels/service.h>
#include <gnunet_channels/channel.h>
#include <cadet_port.h>

using namespace std;
using namespace gnunet_channels;

struct Chat {
    Chat(asio::io_service& ios)
        : was_destroyed(make_shared<bool>(false))
        , _input(ios, ::dup(STDIN_FILENO))
    {}

    void start()
    {
        start_receiving();

        asio::spawn(channel->get_io_service(), [this] (auto yield) {
                string out;
                asio::streambuf buffer(512);

                while (true) {
                    sys::error_code ec;
                    size_t n = asio::async_read_until(_input, buffer, '\n', yield[ec]);
                    if (ec) break;
                    out.resize(n - 1);
                    buffer.sgetn((char*) out.c_str(), n - 1);
                    buffer.consume(1); // new line
                    channel->send(out);
                }
            });
    }

    void start_receiving() {
        channel->receive([this, wd = was_destroyed] (sys::error_code ec, string data) {
                if (*wd) return;
                cout << "Received: " << data << endl;
                start_receiving();
            });
    }

    ~Chat() { *was_destroyed = true; }

    shared_ptr<bool> was_destroyed;
    unique_ptr<Channel> channel;
    asio::posix::stream_descriptor _input;
};

shared_ptr<CadetPort> cadet_port;
shared_ptr<Chat> chat;

static void print_usage(const char* app_name)
{
    cerr << "Usage:\n";
    cerr << "    " << app_name << " <config-file> <secret-phrase> [peer-id]\n";
    cerr << "If [peer-id] is used the app acts as a client, "
            "otherwise it acts as a server\n";
}

int main(int argc, char* const* argv)
{
    if (argc != 3 && argc != 4) {
        print_usage(argc[0]);
        return 1;
    }
    
    asio::io_service ios;
    
    Service service(argv[1], ios);
    
    string target_id;
    string port = argv[2];

    if (argc >= 4) {
        target_id = argv[3];
    }
    
    // Capture these signals so that we can disconnect gracefully.
    asio::signal_set signals(ios, SIGINT, SIGTERM);

    signals.async_wait([&](sys::error_code, int /* signal_number */) {
            if (cadet_port) cadet_port->close();
            cadet_port.reset();
            chat.reset();
        });

    service.async_setup([&target_id, &service, port] (sys::error_code ec) {
             chat = make_shared<Chat>(service.get_io_service());

             if (!target_id.empty()) {
                 cout << "Connecting to " << target_id << endl;

                 chat->channel = make_unique<Channel>(service);
                 chat->channel->connect(target_id, port,
                         [](sys::error_code e) {
                             cout << "Connect " << e.message() << endl;
                             chat->start();
                         });
             }
             else {
                 cout << "Accepting on port \"" << port << "\"" << endl;
                 cadet_port = make_shared<CadetPort>(service);
                 cadet_port->open(port, [] (sys::error_code, auto ch) {
                         cout << "Accepted channel" << endl;
                         chat->channel = make_unique<Channel>(move(ch));
                         chat->start();
                     });
             }
        });
    
    ios.run();
}
