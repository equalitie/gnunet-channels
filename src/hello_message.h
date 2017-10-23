#pragma once

#include <iostream>
#include <assert.h>

namespace gnunet_channels {

class HelloMessage {
public:
    HelloMessage(GNUNET_HELLO_Message* msg);

    HelloMessage(const HelloMessage&) = delete;
    HelloMessage& operator=(const HelloMessage&) = delete;

    HelloMessage(HelloMessage&& m);
    HelloMessage& operator=(HelloMessage&& m);

    GNUNET_PeerIdentity peer_identity() const;

    ~HelloMessage();

private:
    GNUNET_HELLO_Message* _msg = nullptr;
};

//--------------------------------------------------------------------
inline
HelloMessage::HelloMessage(GNUNET_HELLO_Message* msg)
    : _msg(msg)
{ }

inline
HelloMessage::HelloMessage(HelloMessage&& m)
{
    _msg = m._msg;
    m._msg = nullptr;
}

inline
HelloMessage& HelloMessage::operator=(HelloMessage&& m)
{
    _msg = m._msg;
    m._msg = nullptr;
    return *this;
}

inline
GNUNET_PeerIdentity HelloMessage::peer_identity() const
{
    // TODO: Throw errors instead of asserts?
    assert(_msg);
    GNUNET_PeerIdentity pid;
    auto r = GNUNET_HELLO_get_id(_msg, &pid);
    assert(r == GNUNET_OK);
    return pid;
}

inline
HelloMessage::~HelloMessage()
{
    if (_msg) GNUNET_free(_msg);
}

inline
std::ostream& operator<<(std::ostream& os, const GNUNET_PeerIdentity& pid)
{
    return os << GNUNET_i2s_full(&pid);
}

} // gnunet_channels namespace
