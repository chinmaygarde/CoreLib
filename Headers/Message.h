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


#ifndef __CORELIB__MESSAGE__
#define __CORELIB__MESSAGE__

#include "Channel.h"
#include "Attachment.h"

#include <vector>
#include <utility>
#include <memory>
#include <string.h>

namespace cl {

class Message {

  public:
    explicit Message(size_t reservedLength = 0);
    explicit Message(const uint8_t *buffer, size_t bufferLength);

    ~Message();

    bool reserve(size_t length);

    /*
     *  Encoding and decoding common types
     */

    template <typename Type> bool encode(Type value) {

        bool success = reserve(_dataLength + sizeof(Type));

        if (success) {
            memcpy(_buffer + _dataLength, &value, sizeof(Type));
            _dataLength += sizeof(Type);
        }

        return success;
    }

    template <typename Type> bool decode(Type &value) {

        if ((sizeof(Type) + _sizeRead) > _bufferLength) {
            return false;
        }

        memcpy(&value, _buffer + _sizeRead, sizeof(Type));

        _sizeRead += sizeof(Type);

        return true;
    }

    /*
     *  Encoding and decoding special attachments
     */

    void addAttachment(Attachment attachment);

    typedef std::vector<Attachment>::const_iterator AttachmentIterator;
    typedef std::pair<AttachmentIterator, AttachmentIterator> AttachmentRange;

    AttachmentRange attachmentRange() const {
        return std::make_pair(_attachments.begin(), _attachments.end());
    }

    const uint8_t *data() const {
        return _buffer;
    }

    size_t size() const {
        return _dataLength;
    }

    size_t sizeRead() const {
        return _sizeRead;
    }

  private:
    bool resizeBuffer(size_t size);

    uint8_t *_buffer;

    size_t _bufferLength;
    size_t _dataLength;
    size_t _sizeRead;

    std::vector<Attachment> _attachments;

    DISALLOW_COPY_AND_ASSIGN(Message);
};

}

#endif /* defined(__CORELIB__MESSAGE__) */
