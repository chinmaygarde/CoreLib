/*
  The MIT License (MIT)
  
  Copyright (c) 2015, Chinmay Garde
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


#include "NetworkUtilities.h"
#include "Socket.h"
#include "StreamSocket.h"
#include "SeqPacketSocket.h"
#include "Utilities.h"

#if __APPLE__
// For Single Unix Standard v3 (SUSv3) conformance
#define _DARWIN_C_SOURCE
#endif

#include <mutex>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using namespace cl;

const size_t Socket::MaxBufferSize = 4096;
const size_t Socket::MaxControlBufferItemCount = 8;
const size_t Socket::ControlBufferItemSize = sizeof(int);
const size_t Socket::MaxControlBufferSize =
    CMSG_SPACE(ControlBufferItemSize * MaxControlBufferItemCount);

std::unique_ptr<Socket> Socket::Create(Socket::Handle handle) {
    if (NetworkUtil::SocketSeqPacketSupported()) {
        return cl::Utils::make_unique<SeqPacketSocket>(handle);
    } else {
        return cl::Utils::make_unique<StreamSocket>(handle);
    }
}

Socket::Pair Socket::CreatePair() {
    int socketHandles[2] = {0};

    CL_CHECK(::socketpair(AF_UNIX,
                              NetworkUtil::SupportedDomainSocketType(),
                              0,
                              socketHandles));

    return Pair(Socket::Create(socketHandles[0]),
                Socket::Create(socketHandles[1]));
}

Socket::Socket(Handle handle) {

    /*
     *  Create a socket if one is not provided
     */

    if (handle == 0) {
        handle = ::socket(AF_UNIX, NetworkUtil::SupportedDomainSocketType(), 0);
        CL_ASSERT(handle != -1);
    }

    CL_ASSERT(handle > 0);

    /*
     *  Limit the socket send and receive buffer sizes since we dont need large
     *  buffers for channels
     */
    const int size = MaxBufferSize;

    CL_CHECK(
        ::setsockopt(handle, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)));

    CL_CHECK(
        ::setsockopt(handle, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)));

    /*
     *  Make the sockets non blocking since we plan to receive all messages
     *  instead of hitting the waitset for each message in the queue. This is
     *  important since reads will block if we dont.
     */
    CL_CHECK(::fcntl(handle, F_SETFL, O_NONBLOCK));

    CL_CHECK(::fcntl(handle, F_SETFL, O_NONBLOCK));

    /*
     *  Setup the channel buffer
     */
    _buffer = static_cast<uint8_t *>(malloc(MaxBufferSize));
    _controlBuffer = static_cast<uint8_t *>(malloc(MaxControlBufferSize));

    _handle = handle;
};

bool Socket::connect(std::string endpoint) {
    /*
     *  Connect
     */
    CL_ASSERT(endpoint.length() <= 96);

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, endpoint.c_str());

#if __APPLE__
    addr.sun_len = SUN_LEN(&addr);
#endif

    int result = ::connect(_handle,
                           (const struct sockaddr *)&addr,
                           (socklen_t)SUN_LEN(&addr));

    return (result == 0);
}

bool Socket::close() {
    if (_handle != -1) {

        CL_CHECK(::close(_handle));
        _handle = -1;

        return true;
    }

    return false;
}

Socket::~Socket() {
    close();

    free(_buffer);
    free(_controlBuffer);
}
