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

#include "Server.h"
#include "Utilities.h"
#include "NetworkUtilities.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using namespace cl;

Server::Server(std::string name)
    : _name(name), _socketHandle(-1), _listening(false) {
    /*
     *  Step 1: Create the socket
     */
    int socketHandle =
        socket(AF_UNIX, NetworkUtil::SupportedDomainSocketType(), 0);

    if (socketHandle == -1) {
        CL_LOG_ERRNO();
        CL_ASSERT(false);
        return;
    }

    CL_ASSERT(name.length() <= 96);

    /*
     *  Step 2: Bind
     */
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, name.c_str());

#if __APPLE__
    addr.sun_len = SUN_LEN(&addr);
#endif

    int result = 0;

    /*
     *  In case of crashes or incorrect collection of resource, a previous
     *  FS entry might be present. Clear that as seen.
     */
    while (true) {
        /*
         *  Does not return return EINTR (at least what docs say)
         */
        int result = ::bind(socketHandle, 
                            (const struct sockaddr *)&addr,
                            (socklen_t)SUN_LEN(&addr));

        if (result == 0) {
            break;
        }

        if (result == -1 && errno == EADDRINUSE) {
            CL_CHECK(::unlink(name.c_str()));
        }
    }

    if (result == -1) {
        CL_LOG_ERRNO();
        CL_ASSERT(false);
        return;
    }

    /*
     *  Step 3: Listen
     */
    result = ::listen(socketHandle, 1);

    if (result == -1) {
        CL_LOG_ERRNO();
        CL_ASSERT(false);
        return;
    }

    _listening = true;
    _socketHandle = socketHandle;
}

Server::~Server() {
    if (_socketHandle != -1) {
        CL_CHECK(::unlink(_name.c_str()));
        CL_CHECK(::close(_socketHandle));
    }
}

void Server::onConnectionAvailableForAccept(Handle handle) {

    CL_ASSERT(handle == _socketHandle);

    Channel::Handle connectionHandle =
        ::accept(_socketHandle, nullptr, nullptr);

    if (connectionHandle == -1) {
        CL_LOG_ERRNO();
        CL_ASSERT(false);
        return;
    }

    if (!_channelAvailablilityCallback) {
        /*
         *  If the channel availability handler is not set, the server
         *  is listening for connections but won't actually do anything
         *  when one can be established. Instead of keeping clients in limbo,
         *  terminate the connection.
         */
        CL_CHECK(::close(connectionHandle));
        return;
    }

    _channelAvailablilityCallback(std::make_shared<Channel>(connectionHandle));
}

std::shared_ptr<LooperSource> Server::clientConnectionsSource() {
    if (_clientConnectionsSource.get() != nullptr) {
        return _clientConnectionsSource;
    }

    CL_ASSERT(_socketHandle != -1);

    LooperSource::IOHandlesAllocator allocator =
        [&]() { return LooperSource::Handles(_socketHandle, -1); };

    LooperSource::IOHandler readHandler = [&](LooperSource::Handle handle) {
        this->onConnectionAvailableForAccept(handle);
    };

    _clientConnectionsSource = std::make_shared<LooperSource>(allocator,
                                                              nullptr,
                                                              readHandler, 
                                                              nullptr);

    return _clientConnectionsSource;
}
