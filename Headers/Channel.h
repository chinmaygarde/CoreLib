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


#ifndef __CORELIB__CHANNEL__
#define __CORELIB__CHANNEL__

#include "Base.h"
#include "Looper.h"
#include "Socket.h"

#include <string>
#include <memory>

namespace cl {

class Message;
class Channel {

  public:
    typedef int Handle;

    typedef std::function<void(Message &)> MessageReceivedCallback;
    typedef std::function<void(void)> TerminationCallback;
    typedef std::pair<std::shared_ptr<Channel>, std::shared_ptr<Channel>>
        ConnectedChannels;

    /**
     *  Create a channel to a named endpoint. Connection
     *  must be explicitly setup by the caller
     *
     *  @param name the name of the endopoint
     *
     *  @return the channel
     */
    explicit Channel(std::string name);

    /**
     *  Create a channel using a raw channel handle
     *  Ownership of the handle is assumed by the channel
     *  object
     *
     *  @param socketHandle the socket handle
     *
     *  @return the channel
     */
    explicit Channel(Handle socketHandle);

    /**
     *  Create a channel using a preallocated socket
     *  that is already connected
     *
     *  @param socket the socket object
     *
     *  @return the channel
     */
    explicit Channel(std::unique_ptr<Socket> socket);

    static ConnectedChannels CreateConnectedChannels();

    ~Channel();

    /**
     *  Adds the channel to the specified looper.
     *  It is programmer error to collect the channel
     *  before it is unscheduled from all loopers via
     *  `unscheduleFromLooper`
     *
     *  @param looper the looper
     */
    void scheduleInLooper(Looper *looper);

    /**
     *  Unschedule the channel from the specified looper.
     *
     *  @param looper the looper
     */
    void unscheduleFromLooper(Looper *looper);

    /*
     *  Sending and receiving messages
     */

    bool sendMessage(Message &message);

    void messageReceivedCallback(MessageReceivedCallback callback) {
        _messageReceivedCallback = callback;
    }

    MessageReceivedCallback messageReceivedCallback() const {
        return _messageReceivedCallback;
    }

    /*
     *  Introspecting connection status
     */

    bool tryConnect();

    bool isConnected() const {
        return _connected;
    }

    bool isReady() const {
        return _ready;
    }

    void terminate();

    void terminationCallback(TerminationCallback callback) {
        _terminationCallback = callback;
    }

    TerminationCallback terminationCallback() const {
        return _terminationCallback;
    }

  private:
    std::shared_ptr<LooperSource> source();
    std::shared_ptr<LooperSource> _source;

    MessageReceivedCallback _messageReceivedCallback;
    TerminationCallback _terminationCallback;

    std::unique_ptr<Socket> _socket;

    bool _ready;
    bool _connected;

    std::string _name;

    void readMessageOnHandle(Handle handle);

    DISALLOW_COPY_AND_ASSIGN(Channel);
};

}

#endif /* defined(__CORELIB__CHANNEL__) */
