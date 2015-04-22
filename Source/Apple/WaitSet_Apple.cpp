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


#include "WaitSet.h"
#include "Utilities.h"

#include <sys/event.h>
#include <unistd.h>

using namespace cl;

WaitSet::Handle WaitSet::platformHandleCreate() {

    WaitSet::Handle handle = CL_TEMP_FAILURE_RETRY(::kqueue());

    CL_ASSERT(handle != -1);

    return handle;
}

LooperSource *WaitSet::platformHandleWait(WaitSet::Handle handle) {
    struct kevent event = {0};

    int val = CL_TEMP_FAILURE_RETRY(
        ::kevent(handle, nullptr, 0, &event, 1, nullptr));

    CL_ASSERT(val == 1);

    return static_cast<LooperSource *>(event.udata);
}

void WaitSet::platformHandleDestory(WaitSet::Handle handle) {
    CL_CHECK(::close(handle));
}
