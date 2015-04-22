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

#include "SeqPacketSocket.h"
#include "Message.h"
#include "Utilities.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <poll.h>

using namespace cl;

SeqPacketSocket::SeqPacketSocket(Handle handle) : Socket(handle) {

}

SeqPacketSocket::~SeqPacketSocket() {

}

Socket::Status SeqPacketSocket::WriteMessage(Message &message) {
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

Socket::ReadResult SeqPacketSocket::ReadMessages() {
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
