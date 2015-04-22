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


#include "StreamSocket.h"
#include "Message.h"
#include "Utilities.h"

#if __APPLE__
// For Single Unix Standard v3 (SUSv3) conformance
#define _DARWIN_C_SOURCE
#endif

#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <poll.h>

using namespace cl;

namespace cl {
typedef struct { uint16_t size; } FramingHeader;
}

StreamSocket::StreamSocket(Socket::Handle handle) : Socket(handle) {
    _unprocessedDataSize = 0;
}

StreamSocket::~StreamSocket() {
}

Socket::Status StreamSocket::WriteMessage(Message &message) {
    AutoLock lock(_lock);

    if (message.size() + sizeof(FramingHeader) > MaxBufferSize) {
        return Status::TemporaryFailure;
    }

    size_t totalSize = 0; /* for error checking */

    FramingHeader header = {
        .size = static_cast<uint16_t>(message.size()),
    };

    /*
     *  Setup the scatter/gather array of data
     */

    struct iovec vec[2] = {{0}};

    vec[0].iov_base = &header;
    vec[0].iov_len = sizeof(header);

    totalSize += vec[0].iov_len;

    vec[1].iov_base = (void *)message.data();
    vec[1].iov_len = message.size();

    totalSize += vec[1].iov_len;

    /*
     *  Setup control messages for sending descriptors
     */

    struct msghdr msgh = {0};

    msgh.msg_iov = vec;
    msgh.msg_iovlen = sizeof(vec) / sizeof(struct iovec);

    auto range = message.attachmentRange();

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
        msgh.msg_control = buffer;
        msgh.msg_controllen = controlLength;

        struct cmsghdr *cmsgh = CMSG_FIRSTHDR(&msgh);

        cmsgh->cmsg_level = SOL_SOCKET;
        cmsgh->cmsg_type = SCM_RIGHTS;
        cmsgh->cmsg_len = CMSG_LEN(sizeof(descriptors));

        memcpy((int *)CMSG_DATA(cmsgh), descriptors, sizeof(descriptors));

        msgh.msg_controllen = cmsgh->cmsg_len;
    }

    int sent = 0;

    while (true) {
        sent = CL_TEMP_FAILURE_RETRY(::sendmsg(_handle, &msgh, 0));

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

    if (sent != totalSize) {
        return errno == EPIPE ? Status::PermanentFailure
                              : Status::TemporaryFailure;
    }

    return Status::Success;
}

std::vector<std::unique_ptr<Message>>
StreamSocket::processReadBuffer(ssize_t received) {

    std::vector<std::unique_ptr<Message>> messages;

    ssize_t bufferBaseOffset = 0;
    ssize_t bytesToRead = received;

    while (true) {
        /*
         *  Read the framing header
         */
        FramingHeader header = {0};

        uint8_t *buffer = _buffer + bufferBaseOffset;

        if (bytesToRead < sizeof(FramingHeader)) {
            break;
        }

        memcpy(&header, buffer, sizeof(FramingHeader));

        /*
         *  Figure out the start of the message data
         */

        uint8_t *messageData = buffer + sizeof(FramingHeader);

        if (bytesToRead - sizeof(FramingHeader) < header.size) {
            break;
        }

        messages.push_back(
            cl::Utils::make_unique<Message>(messageData, header.size));

        /*
         *  Once the message is constructed, it is consumed
         */

        ssize_t consumed = header.size + sizeof(FramingHeader);

        bytesToRead -= consumed;
        bufferBaseOffset += consumed;
    }

    if (bytesToRead > 0) {
        /*
         *  There was buffered data but it did not form a completely
         *  framed message.
         */
        memmove(_buffer, _buffer + bufferBaseOffset, bytesToRead);
    }

    CL_ASSERT(bytesToRead >= 0);
    _unprocessedDataSize = bytesToRead;

    return messages;
}

Socket::ReadResult StreamSocket::ReadMessages() {
    AutoLock lock(_lock);

    struct iovec vec[1] = {{0}};

    CL_ASSERT(MaxBufferSize > _unprocessedDataSize);

    vec[0].iov_base = _buffer + _unprocessedDataSize;
    vec[0].iov_len = MaxBufferSize - _unprocessedDataSize;

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

    if (received == -1) {
        CL_LOG_ERRNO();
        return ReadResult(Status::TemporaryFailure,
                          std::vector<std::unique_ptr<Message>>());
    }

    if (received == 0) {
        /*
         *  if no messages are available to be received and the peer
         *  has performed an orderly shutdown, recvmsg() returns 0
         */
        return ReadResult(Status::PermanentFailure,
                          std::vector<std::unique_ptr<Message>>());
    }

    /*
     *  Check if the message contains descriptors
     */
    if (messageHeader.msg_controllen != 0) {
        struct cmsghdr *cmsgh = nullptr;

        while ((cmsgh = CMSG_NXTHDR(&messageHeader, cmsgh)) != nullptr) {
            CL_ASSERT(cmsgh->cmsg_level == SOL_SOCKET);
            CL_ASSERT(cmsgh->cmsg_type == SCM_RIGHTS);
            CL_ASSERT(cmsgh->cmsg_len == CMSG_LEN(ControlBufferItemSize));

            int descriptor = -1;
            memcpy(&descriptor, CMSG_DATA(cmsgh), ControlBufferItemSize);

            CL_ASSERT(descriptor != -1);
        }

        /*
         *  Since we dont handle partial writes of the control messages
         *  assert that the same was not truncated.
         */
        CL_ASSERT((messageHeader.msg_flags & MSG_CTRUNC) == 0);
    }

    return ReadResult(Status::Success, processReadBuffer(received));
}
