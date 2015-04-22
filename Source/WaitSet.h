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

#ifndef __CORELIB__WAITSET__
#define __CORELIB__WAITSET__

#include <set>

#include "Base.h"

namespace cl {

class LooperSource;

class WaitSet {

  public:
    typedef int Handle;

    WaitSet();
    ~WaitSet();

    bool addSource(std::shared_ptr<LooperSource> source);
    bool removeSource(std::shared_ptr<LooperSource> source);

    LooperSource *wait();

  private:
    Handle _handle;

    static Handle platformHandleCreate();
    static LooperSource *platformHandleWait(Handle handle);
    static void platformHandleDestory(Handle handle);

    std::set<std::shared_ptr<LooperSource>> _sources;

    DISALLOW_COPY_AND_ASSIGN(WaitSet);
};

}

#endif /* defined(__CORELIB__WAITSET__) */
