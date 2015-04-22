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

#include "Message.h"
#include "Utilities.h"

#include <stdlib.h>

using namespace cl;

Message::Message(size_t length)
    : _buffer(nullptr), _bufferLength(0), _dataLength(0), _sizeRead(0) {

    reserve(length);
}

Message::Message(const uint8_t *buffer, size_t bufferSize)
    : _dataLength(bufferSize), _bufferLength(bufferSize), _sizeRead(0) {

    void *allocation = malloc(bufferSize);

    memcpy(allocation, buffer, bufferSize);

    _buffer = static_cast<uint8_t *>(allocation);
}

Message::~Message() {
    free(_buffer);
}

static inline size_t Message_NextPOTSize(size_t x) {
    --x;

    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;

    return x + 1;
}

bool Message::reserve(size_t length) {
    if (length == 0 || _bufferLength >= length) {
        return true;
    }

    return resizeBuffer(Message_NextPOTSize(length));
}

bool Message::resizeBuffer(size_t size) {
    if (_buffer == nullptr) {
        CL_ASSERT(_bufferLength == 0);

        void *buffer = malloc(size);

        bool success = buffer != nullptr;

        if (success) {
            _buffer = static_cast<uint8_t *>(buffer);
            _bufferLength = size;
        }

        return success;
    }

    CL_ASSERT(size > _bufferLength);

    void *resized = realloc(_buffer, size);

    bool success = resized != nullptr;

    if (success) {
        _buffer = static_cast<uint8_t *>(resized);
        _bufferLength = size;
    }

    return success;
}

void Message::addAttachment(Attachment attachment) {
    CL_ASSERT(_attachments.size() <= Socket::MaxControlBufferItemCount);
    
    _attachments.push_back(attachment);
}
