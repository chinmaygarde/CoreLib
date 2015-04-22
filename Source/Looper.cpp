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


#include "Looper.h"
#include "Utilities.h"

#include <pthread.h>
#include <mutex>

using namespace cl;

Looper *Looper::Current() {
    static std::once_flag once;
    static pthread_key_t LooperTLSKey;

    std::call_once(once, []() {
        pthread_key_create(&LooperTLSKey, [](void *looper) {
            delete static_cast<Looper *>(looper);
        });
    });

    auto currentLooper =
        static_cast<Looper *>(pthread_getspecific(LooperTLSKey));

    if (currentLooper == nullptr) {
        currentLooper = new Looper();
        pthread_setspecific(LooperTLSKey, currentLooper);
    }

    return currentLooper;
}

Looper::Looper() : _shouldTerminate(false) {
}

Looper::~Looper() {
}

bool Looper::addSource(std::shared_ptr<LooperSource> source) {
    return _waitSet.addSource(source);
}

bool Looper::removeSource(std::shared_ptr<LooperSource> source) {
    return _waitSet.removeSource(source);
}

void Looper::loop() {

    if (_trivialSource.get() == nullptr) {
        /*
         *  A trivial source needs to be added to keep the loop idle
         *  without any other sources present.
         */
        _trivialSource = LooperSource::AsTrivial();
        addSource(_trivialSource);
    }

    while (!_shouldTerminate) {
        LooperSource *source = _waitSet.wait();

        if (source == nullptr) {
            continue;
        }

        auto reader = source->reader();

        if (reader) {
            reader(source->readHandle());
        }

        source->onAwoken();
    }

    _shouldTerminate = false;
}

void Looper::terminate() {
    _shouldTerminate = true;
    _trivialSource->writer()(_trivialSource->writeHandle());
}
