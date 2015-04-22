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

#include <sys/event.h>

using namespace cl;

static inline void LooperSource_UpdateKeventSource(int queue, uintptr_t ident,
                                                   int16_t filter,
                                                   uint16_t flags,
                                                   uint32_t fflags,
                                                   intptr_t data, void *udata) {
    struct kevent event = {0};

    EV_SET(&event,
           ident,
           filter,
           flags,
           fflags,
           data,
           udata);

    CL_TEMP_FAILURE_RETRY(::kevent(queue, &event, 1, nullptr, 0, NULL));
}

void LooperSource::updateInWaitSetHandle(WaitSet::Handle waitsetHandle,
                                         bool shouldAdd) {
    if (_customWaitSetUpdateHandler) {
        _customWaitSetUpdateHandler(this,
                                    waitsetHandle,
                                    readHandle(),
                                    shouldAdd);
        return;
    }

    LooperSource_UpdateKeventSource(waitsetHandle,
                                    readHandle(),
                                    EVFILT_READ,
                                    shouldAdd ? EV_ADD : EV_DELETE,
                                    0,
                                    0,
                                    this);
}

std::shared_ptr<LooperSource>
LooperSource::AsTimer(std::chrono::nanoseconds repeatInterval) {

    WaitSetUpdateHandler updateHandler =
        [repeatInterval](LooperSource *source, WaitSet::Handle waitsetHandle,
                         Handle readHandle, bool adding) {

            LooperSource_UpdateKeventSource(waitsetHandle,
                                            readHandle,
                                            EVFILT_TIMER,
                                            adding ? EV_ADD : EV_DELETE,
                                            NOTE_NSECONDS,
                                            repeatInterval.count(),
                                            source);

        };

    auto timer = std::make_shared<LooperSource>(nullptr,
                                                nullptr,
                                                nullptr,
                                                nullptr);

    timer->setCustomWaitSetUpdateHandler(updateHandler);

    return timer;
}
