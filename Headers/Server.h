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

#ifndef __CORELIB__SERVER__
#define __CORELIB__SERVER__

#include "Base.h"

#include <string>
#include <functional>

#include "LooperSource.h"
#include "Channel.h"

namespace cl {

class Server {
  public:
    typedef int Handle;

    typedef std::function<void(std::shared_ptr<Channel>)>
        ChannelAvailabilityCallback;

    explicit Server(std::string name);
    ~Server();

    std::shared_ptr<LooperSource> clientConnectionsSource();

    /*
     *  Server Status
     */

    bool isListening() const {
        return _listening;
    }

    void channelAvailabilityCallback(ChannelAvailabilityCallback callback) {
        _channelAvailablilityCallback = callback;
    }

    ChannelAvailabilityCallback channelAvailabilityCallback() const {
        return _channelAvailablilityCallback;
    }

  private:
    std::string _name;

    Handle _socketHandle;
    bool _listening;

    std::shared_ptr<LooperSource> _clientConnectionsSource;

    ChannelAvailabilityCallback _channelAvailablilityCallback;

    void onConnectionAvailableForAccept(Handle handle);

    DISALLOW_COPY_AND_ASSIGN(Server);
};

}

#endif /* defined(__CORELIB__SERVER__) */
