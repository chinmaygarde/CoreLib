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


#include "Attachment.h"
#include "Channel.h"
#include "Message.h"
#include "SharedMemory.h"

#include <gtest/gtest.h>
#include <thread>

TEST(AttachmentTest, SingleAttachment) {

    auto channels = cl::Channel::CreateConnectedChannels();

    std::thread serverThread([&channels]() {

        /*
         *  Put the message in shared memory
         */
        const char hello[] = "hello";

        cl::SharedMemory memory(sizeof(hello));

        memcpy(memory.address(), hello, sizeof(hello));

        /*
         *  Create and send the shared memory handle as an attachment
         */
        cl::Message message;

        cl::Attachment attachment(memory.handle());
        message.addAttachment(attachment);

        ASSERT_TRUE(channels.first->sendMessage(message));
    });

    std::thread cliendThread([&channels]() {

        auto &channel = channels.second;

        channel->messageReceivedCallback([](cl::Message &message) {
            auto attachmentRange = message.attachmentRange();

            auto count = attachmentRange.second - attachmentRange.first;
            ASSERT_TRUE(count == 1);

            cl::Looper::Current()->terminate();
        });

        auto looper = cl::Looper::Current();

        channel->scheduleInLooper(looper);
        looper->loop();
    });

    serverThread.join();
    cliendThread.join();
}

