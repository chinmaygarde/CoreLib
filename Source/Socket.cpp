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

#include "Socket.h"
#include "Utilities.h"
#include "Attachment.h"
#include "Message.h"

#if __APPLE__
// For Single Unix Standard v3 (SUSv3) conformance
#define _DARWIN_C_SOURCE
#endif

#include <mutex>

#include <fcntl.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <errno.h>

#include <poll.h>

using namespace cl;

const size_t Socket::MaxBufferSize = 4096;
const size_t Socket::MaxControlBufferItemCount = 8;
const size_t Socket::ControlBufferItemSize = sizeof(int);
const size_t Socket::MaxControlBufferSize =
    CMSG_SPACE(ControlBufferItemSize * MaxControlBufferItemCount);

std::unique_ptr<Socket> Socket::Create(Socket::Handle handle) {
    return cl::Utils::make_unique<Socket>(handle);
}

Socket::Pair Socket::CreatePair() {
    int socketHandles[2] = {0};

    CL_CHECK(::socketpair(AF_UNIX,
                              SOCK_SEQPACKET,
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
        handle = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
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

Socket::ReadResult Socket::ReadMessages() {
    AutoLock lock(_lock);

    struct iovec vec[1] = {{0}};

    vec[0].iov_base = _buffer;
    vec[0].iov_len = MaxBufferSize;

    std::vector<std::unique_ptr<Message>> messages;

    while (true) {

        struct msghdr messageHeader = {
            .msg_name = nullptr,
            .msg_namelen = 0,
            .msg_iov = vec,
            .msg_iovlen = sizeof(vec) / sizeof(struct iovec),
            .msg_control = _controlBuffer,
            .msg_controllen = static_cast<socklen_t>(MaxControlBufferSize),
            .msg_flags = 0,
        };

        int received =
            CL_TEMP_FAILURE_RETRY(::recvmsg(_handle, &messageHeader, 0));

        if (received > 0) {

            auto message =
                cl::Utils::make_unique<Message>(_buffer, received);

            /*
             *  Check if the message contains descriptors.
             *  Read all descriptors in one go
             */
            if (messageHeader.msg_controllen != 0) {
                struct cmsghdr *cmsgh = nullptr;

                while ((cmsgh = CMSG_NXTHDR(&messageHeader, cmsgh)) !=
                       nullptr) {
                    CL_ASSERT(cmsgh->cmsg_level == SOL_SOCKET);
                    CL_ASSERT(cmsgh->cmsg_type == SCM_RIGHTS);
                    CL_ASSERT(cmsgh->cmsg_len ==
                                  CMSG_LEN(ControlBufferItemSize));

                    int descriptor = -1;
                    memcpy(&descriptor, CMSG_DATA(cmsgh),
                           ControlBufferItemSize);

                    CL_ASSERT(descriptor != -1);

                    Attachment attachment(descriptor);
                    message->addAttachment(attachment);
                }
            }

            /*
             *  Since we dont handle partial writes of the control messages,
             *  assert that the same was not truncated.
             */
            CL_ASSERT((messageHeader.msg_flags & MSG_CTRUNC) == 0);

            /*
             *  Finally! We have the message and possible attachments. Try for
             * more.
             */
            messages.push_back(std::move(message));

            continue;
        }

        if (received == -1) {
            /*
             *  All pending messages have been read. poll for more
             *  in subsequent calls. We are finally done!
             */
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /*
                 *  Return as a successful read
                 */
                break;
            }

            return ReadResult(Status::TemporaryFailure, std::move(messages));
        }

        if (received == 0) {
            /*
             *  if no messages are available to be received and the peer
             *  has performed an orderly shutdown, recvmsg() returns 0
             */
            return ReadResult(Status::PermanentFailure, std::move(messages));
        }
    }

    return ReadResult(Status::Success, std::move(messages));
}

Socket::Status Socket::WriteMessage(Message &message) {
    AutoLock lock(_lock);

    if (message.size() > MaxBufferSize) {
        return Status::TemporaryFailure;
    }

    struct iovec vec[1] = {{0}};

    vec[0].iov_base = (void *)message.data();
    vec[0].iov_len = message.size();

    struct msghdr messageHeader = {
        .msg_name = nullptr,
        .msg_namelen = 0,
        .msg_iov = vec,
        .msg_iovlen = sizeof(vec) / sizeof(struct iovec),
        .msg_control = nullptr,
        .msg_controllen = 0,
        .msg_flags = 0,
    };

    const auto range = message.attachmentRange();

    const size_t descriptorCount = range.second - range.first;

    if (descriptorCount > 0) {

        /*
         *  Create an array of file descriptors
         */

        int descriptors[descriptorCount];

        size_t index = 0;
        for (auto i = range.first; i != range.second; i++) {
            descriptors[index++] = (*i).handle();
        }

        auto controlLength = CMSG_SPACE(sizeof(descriptors));

        char buffer[controlLength];
        messageHeader.msg_control = buffer;
        messageHeader.msg_controllen = controlLength;

        struct cmsghdr *cmsgh = CMSG_FIRSTHDR(&messageHeader);

        cmsgh->cmsg_level = SOL_SOCKET;
        cmsgh->cmsg_type = SCM_RIGHTS;
        cmsgh->cmsg_len = CMSG_LEN(sizeof(descriptors));

        memcpy((int *)CMSG_DATA(cmsgh), descriptors, sizeof(descriptors));

        messageHeader.msg_controllen = cmsgh->cmsg_len;
    }

    int sent = 0;

    while (true) {
        sent = CL_TEMP_FAILURE_RETRY(::sendmsg(_handle, &messageHeader, 0));

        if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            struct pollfd pollFd = {
                .fd = _handle, .events = POLLOUT, .revents = 0,
            };

            int res =
                CL_TEMP_FAILURE_RETRY(::poll(&pollFd, 1, -1 /* timeout */));

            CL_ASSERT(res == 1);
        } else {
            break;
        }
    }

    if (sent != message.size()) {
        return errno == EPIPE ? Status::PermanentFailure
                              : Status::TemporaryFailure;
    }

    return Status::Success;
}
