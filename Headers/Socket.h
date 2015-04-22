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


#ifndef __CORELIB__SOCKETACCESSOR__
#define __CORELIB__SOCKETACCESSOR__

#include "Base.h"
#include "AutoLock.h"

#include <utility>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <stdint.h>

namespace cl {

class Message;

class Socket {

  public:
    static const size_t MaxBufferSize;
    static const size_t MaxControlBufferItemCount;
    static const size_t ControlBufferItemSize;
    static const size_t MaxControlBufferSize;

    typedef int Handle;

    typedef enum {
        Success = 0,
        TemporaryFailure,
        PermanentFailure,
    } Status;

    typedef std::pair<Status, std::vector<std::unique_ptr<Message>>> ReadResult;
    typedef std::pair<std::unique_ptr<Socket>, std::unique_ptr<Socket>> Pair;

    /*
     *  Creating a socket
     */

    static std::unique_ptr<Socket> Create(Handle Handle = 0);
    static Pair CreatePair();

    Socket() = delete;
    explicit Socket(Handle handle = 0);

    virtual ~Socket();

    /*
     *  Connecting and disconnecting sockets
     */

    bool connect(std::string endpoint);

    bool close();

    /*
     *  Reading and writing messages on sockets
     */

    Status WriteMessage(Message &message);
    ReadResult ReadMessages();

    Handle handle() const {
        return _handle;
    }

  protected:
    Lock _lock;

    uint8_t *_buffer;
    uint8_t *_controlBuffer;

    Handle _handle;
};

}

#endif /* defined(__CORELIB__SOCKETACCESSOR__) */
