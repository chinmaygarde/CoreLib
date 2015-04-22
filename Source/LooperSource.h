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


#ifndef __CORELIB__LOOPERSOURCE__
#define __CORELIB__LOOPERSOURCE__

#include "Base.h"

#include <functional>
#include <utility>
#include <chrono>

#include "WaitSet.h"

namespace cl {

class LooperSource {

  public:
    typedef int Handle;
    typedef std::pair<Handle, Handle> Handles;

    typedef std::function<void(Handle)> IOHandler;

    typedef std::function<void(void)> WakeFunction;

    typedef std::function<Handles(void)> IOHandlesAllocator;
    typedef std::function<void(Handles)> IOHandlesDeallocator;

    typedef std::function<void(LooperSource *source,
                               WaitSet::Handle waitsetHandle, Handle readHandle,
                               bool adding)> WaitSetUpdateHandler;

    LooperSource(IOHandlesAllocator handleAllocator,
                 IOHandlesDeallocator handleDeallocator, IOHandler readHandler,
                 IOHandler writeHandler) {

        _handlesAllocator = handleAllocator;
        _handlesDeallocator = handleDeallocator;

        _customWaitSetUpdateHandler = nullptr;

        _readHandler = readHandler;
        _writeHandler = writeHandler;

        _wakeFunction = nullptr;
        _handles = Handles(-1, -1);
        _handlesAllocated = false;
    }

    ~LooperSource() {
        if (_handlesAllocated && _handlesDeallocator != nullptr) {
            _handlesDeallocator(_handles);
        }
    }

    void onAwoken() {
        if (_wakeFunction != nullptr) {
            _wakeFunction();
        }
    }

    /*
     *  Wait Function
     */

    void setWakeFunction(WakeFunction wakeFunction) {
        _wakeFunction = wakeFunction;
    }

    WakeFunction wakeFunction() const {
        return _wakeFunction;
    }

    /*
     *  Accessing the handles
     */

    Handles handles() {
        /*
         *  Handles are allocated lazily
         */
        if (!_handlesAllocated) {
            auto allocator = _handlesAllocator;

            if (allocator != nullptr) {
                _handles = _handlesAllocator();
            }

            _handlesAllocated = true;
        }

        return _handles;
    }

    /*
     *  Reading the Signalled Source
     */

    Handle readHandle() {
        return handles().first;
    }

    IOHandler reader() {
        return _readHandler;
    }

    /*
     *  Writing to the Source for Signalling
     */

    Handle writeHandle() {
        return handles().second;
    }

    IOHandler writer() {
        return _writeHandler;
    }

    /*
     *  Interacting with a WaitSet
     */
    void updateInWaitSetHandle(WaitSet::Handle handle, bool shouldAdd);

    void setCustomWaitSetUpdateHandler(WaitSetUpdateHandler handler) {
        _customWaitSetUpdateHandler = handler;
    }

    WaitSetUpdateHandler customWaitSetUpdateHandler() const {
        return _customWaitSetUpdateHandler;
    }

    /*
     *  Utility methods for creating commonly used sources
     */

    static std::shared_ptr<LooperSource>
    AsTimer(std::chrono::nanoseconds repeatInterval);

    static std::shared_ptr<LooperSource> AsTrivial();

  private:
    IOHandlesAllocator _handlesAllocator;
    IOHandlesDeallocator _handlesDeallocator;

    Handles _handles;

    IOHandler _readHandler;
    IOHandler _writeHandler;

    WaitSetUpdateHandler _customWaitSetUpdateHandler;

    WakeFunction _wakeFunction;

    bool _handlesAllocated;

    DISALLOW_COPY_AND_ASSIGN(LooperSource);
};

}

#endif /* defined(__CORELIB__LOOPERSOURCE__) */
