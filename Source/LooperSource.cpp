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

#include <unistd.h>

using namespace cl;

std::shared_ptr<LooperSource> LooperSource::AsTrivial() {
    /*
     *  We are using a simple pipe but this should likely be something
     *  that coalesces multiple writes. Something like an event_fd on Linux
     */

    IOHandlesAllocator allocator = [] {
        int descriptors[2] = {0};

        CL_CHECK(::pipe(descriptors));

        return Handles(descriptors[0], descriptors[1]);
    };

    IOHandlesDeallocator deallocator = [](Handles h) {
        CL_CHECK(::close(h.first));
        CL_CHECK(::close(h.second));
    };

    static const char LooperWakeMessage[] = "w";

    IOHandler reader = [](Handle r) {
        char buffer[sizeof(LooperWakeMessage) / sizeof(char)];

        ssize_t size =
            CL_TEMP_FAILURE_RETRY(::read(r, &buffer, sizeof(buffer)));

        CL_ASSERT(size == sizeof(buffer));
    };

    IOHandler writer = [](Handle w) {

        ssize_t size = CL_TEMP_FAILURE_RETRY(
            ::write(w, LooperWakeMessage, sizeof(LooperWakeMessage)));

        CL_ASSERT(size == sizeof(LooperWakeMessage));
    };

    return std::make_shared<LooperSource>(allocator,
                                          deallocator,
                                          reader,
                                          writer);
}
