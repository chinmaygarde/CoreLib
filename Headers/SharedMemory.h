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


#ifndef __CORELIB__SHAREDMEMORY__
#define __CORELIB__SHAREDMEMORY__

#include "Base.h"
#include <string>

namespace cl {

typedef int Handle;

class SharedMemory {

  public:
    explicit SharedMemory(size_t size);
    ~SharedMemory();

    void cleanup();

    bool isReady() const {
        return _ready;
    }

    void *address() const {
        return _address;
    }

    size_t size() const {
        return _size;
    }

    Handle handle() const {
        return _handle;
    }

  private:
    Handle _handle;
    size_t _size;
    void *_address;

    bool _ready;

    DISALLOW_COPY_AND_ASSIGN(SharedMemory);
};

}

#endif /* defined(__CORELIB__SHAREDMEMORY__) */