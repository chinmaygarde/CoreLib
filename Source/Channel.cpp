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


#include "Channel.h"
#include "Message.h"
#include "Utilities.h"

using namespace cl;

Channel::Channel(std::string name) : _name(name), _connected(false) {
    _socket = Socket::Create();
    _ready = true;
}

Channel::Channel(Handle handle) : _ready(true), _connected(true) {
    _socket = Socket::Create(handle);
}

Channel::Channel(std::unique_ptr<Socket> socket)
    : _ready(true), _connected(true), _socket(std::move(socket)) {
}

Channel::ConnectedChannels Channel::CreateConnectedChannels() {
    auto socketPair = Socket::CreatePair();

    return ConnectedChannels(
        std::make_shared<Channel>(std::move(socketPair.first)),
        std::make_shared<Channel>(std::move(socketPair.second)));
}

void Channel::terminate() {

    bool closed = _socket->close();

    _connected = false;
    _ready = false;

    if (closed && _terminationCallback) {
        _terminationCallback();
    }
}

Channel::~Channel() {
    terminate();
}

bool Channel::tryConnect() {

    if (_connected) {
        return true;
    }

    _connected = _socket->connect(_name);

    return _connected;
}

std::shared_ptr<LooperSource> Channel::source() {
    if (_source.get() != nullptr) {
        return _source;
    }

    CL_ASSERT(_connected == true);

    Handle handle = _socket->handle();

    using LS = LooperSource;

    LS::IOHandlesAllocator allocator = [handle]() {
        return LS::Handles(handle, handle); /* bi-di connection */
    };

    LS::IOHandler readHandler =
        [this](LS::Handle handle) { this->readMessageOnHandle(handle); };

    /**
     *  We are specifying a null write handler since we will
     *  never directly signal this source. Instead, we will write
     *  to the handle directly.
     *
     *  The channel own the socket handle, so there is no deallocation
     *  callback either.
     */
    _source = std::make_shared<LS>(allocator, nullptr, readHandler, nullptr);

    return _source;
}

bool Channel::sendMessage(Message &message) {
    CL_ASSERT(message.size() <= 1024);

    Socket::Status writeStatus = _socket->WriteMessage(message);

    if (writeStatus == Socket::Status::PermanentFailure) {
        /*
         *  If the channel is unrecoverable, we need to get rid of our
         *  reference to the descriptor and warn the user of the failure.
         */
        terminate();
        return false;
    }

    return writeStatus == Socket::Status::Success;
}

void Channel::readMessageOnHandle(Handle handle) {
    Socket::Status status;
    std::vector<std::unique_ptr<Message>> messages;

    std::tie(status, messages) = _socket->ReadMessages();

    /*
     *  Dispatch all successfully read messages
     */
    if (_messageReceivedCallback) {
        for (auto &message : messages) {
            _messageReceivedCallback(*message.get());
        }
    }

    /*
     *  On fatal errors, terminate the channel
     */
    if (status == Socket::Status::PermanentFailure) {
        terminate();
        return;
    }
}

void Channel::scheduleInLooper(Looper *looper) {
    if (looper == nullptr || !_connected) {
        return;
    }

    looper->addSource(source());
}

void Channel::unscheduleFromLooper(Looper *looper) {
    if (looper == nullptr) {
        return;
    }

    /* don't invoke the accessor which implicitly constructs a
     source */
    looper->removeSource(_source);
}
