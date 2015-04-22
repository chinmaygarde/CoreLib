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

#include "LooperSource.h"
#include "Utilities.h"
#include "Base.h"

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000  /* nanoseconds per second */
#endif

using namespace cl;

static inline void LooperSource_UpdateEpollSource(int eventsMask, void *data,
                                                  int epollDesc, int operation,
                                                  int desc) {
    struct epoll_event event = {0};

    event.events = eventsMask;
    event.data.ptr = data; /* union */

    CL_TEMP_FAILURE_RETRY(::epoll_ctl(epollDesc, operation, desc, &event));
}

void LooperSource::updateInWaitSetHandle(WaitSet::Handle waitsetHandle,
                                         bool shouldAdd) {

    if (_customWaitSetUpdateHandler) {
        _customWaitSetUpdateHandler(this, waitsetHandle, readHandle(),
                                    shouldAdd);
        return;
    }

    LooperSource_UpdateEpollSource(EPOLLIN,
                                   this,
                                   waitsetHandle,
                                   shouldAdd ? EPOLL_CTL_ADD : EPOLL_CTL_DEL,
                                   readHandle());
}

std::shared_ptr<LooperSource>
LooperSource::AsTimer(std::chrono::nanoseconds repeatInterval) {
    IOHandlesAllocator allocator = [repeatInterval]() {

        /*
         *  Create and arm the timer file descriptor
         */
        Handle desc = timerfd_create(CLOCK_MONOTONIC, 0);

        const uint64_t nano_secs = repeatInterval.count();

        struct itimerspec spec = {0};

        spec.it_value.tv_sec = (time_t)(nano_secs / NSEC_PER_SEC);
        spec.it_value.tv_nsec = nano_secs % NSEC_PER_SEC;

        spec.it_interval = spec.it_value;

        CL_CHECK(::timerfd_settime(desc, 0, &spec, nullptr));

        return Handles(desc, -1);
    };

    IOHandlesDeallocator deallocator = [](Handles handles) {
        CL_ASSERT(handles.second == -1 /* since we never assigned one */);
        CL_CHECK(::close(handles.first));
    };

    IOHandler reader = [](Handle r) {
        /*
         *  8 bytes must be read from a signalled timer file descriptor when
         *  signalled.
         */
        uint64_t fireCount = 0;

        ssize_t size =
            CL_TEMP_FAILURE_RETRY(::read(r, &fireCount, sizeof(uint64_t)));

        CL_ASSERT(size == sizeof(uint64_t));
    };


    return std::make_shared<LooperSource>(allocator,
                                          deallocator,
                                          reader,
                                          nullptr);
}
