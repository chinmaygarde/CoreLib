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

#include <gtest/gtest.h>
#include "Channel.h"
#include "Server.h"
#include "Looper.h"
#include "Message.h"

#include <thread>

TEST(ChannelTest, SimpleInitialization) {

    auto endpoint = "/tmp/corelib_simple_client_server";

    cl::Channel channel(endpoint);
    ASSERT_TRUE(channel.isReady());
}

TEST(ChannelTest, SimpleConnection) {

    auto endpoint = "/tmp/corelib_simple_client_server2";

    cl::Server server(endpoint);

    ASSERT_TRUE(server.isListening());

    std::thread serverThread([&] {
        auto looper = cl::Looper::Current();

        /*
         *  Step 1: Add the source listening for new connections
         *  on the server side
         */
        looper->addSource(server.clientConnectionsSource());

        /*
         *  Step 2: Setup the client that will terminate the server
         *  loop when it connects with the same.
         */
        cl::Channel channel(endpoint);
        ASSERT_TRUE(channel.isReady());

        std::thread clientThread([&] {
            for (int i = 0; i < 5; i++) {
                if (channel.tryConnect()) {
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            ASSERT_TRUE(channel.isConnected());

            looper->terminate();
        });

        /*
         *  Step 3: Start looping the server
         */
        looper->loop();

        clientThread.join();
    });

    serverThread.join();
}

TEST(ChannelTest, SimpleConnectionSend) {

    auto endpoint = "/tmp/corelib_simple_client_server3";

    cl::Server server(endpoint);

    std::shared_ptr<cl::Channel> serverChannelToClient;

    /*
     *  We need to keep the channel to the client open long enough for
     *  it to send us a message. If we depend on the implicit behavior
     *  of closing the connection, we may get into a racy condition where
     *  the ack has happened but the accept has not.
     */
    server.channelAvailabilityCallback(
        [&serverChannelToClient](std::shared_ptr<cl::Channel> channel) {
            serverChannelToClient.swap(channel);

            /*
             *  If the server only wanted one connected, it would terminate
             * these as it came along.
             */
        });

    ASSERT_TRUE(server.isListening());

    std::thread serverThread([&] {
        auto looper = cl::Looper::Current();

        /*
         *  Step 1: Add the source listening for new connections
         *  on the server side
         */
        looper->addSource(server.clientConnectionsSource());

        /*
         *  Step 2: Setup the client that will terminate the server
         *  loop when it connects with the same.
         */
        cl::Channel channel(endpoint);

        ASSERT_TRUE(channel.isReady());

        std::thread clientThread([&] {
            for (int i = 0; i < 5; i++) {
                if (channel.tryConnect()) {
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            ASSERT_TRUE(channel.isConnected());

            cl::Message message;
            message.encode(10.0);
            message.encode(false);
            message.encode('c');

            ASSERT_TRUE(channel.sendMessage(message));

            looper->terminate();
        });

        /*
         *  Step 3: Start looping the server
         */
        looper->loop();

        clientThread.join();
    });

    serverThread.join();

    ASSERT_TRUE(serverChannelToClient.get() != nullptr);

    serverChannelToClient->terminate();
}

TEST(ChannelTest, TwoWayCommunicationTest) {

    auto endpoint = "/tmp/corelib_simple_client_server4";

    int messagesReceived = 0;

    const int MessagesCount = 25;

    std::thread serverThread([endpoint, &messagesReceived]() {
        cl::Server server(endpoint);
        ASSERT_TRUE(server.isListening());

        std::shared_ptr<cl::Channel> clientToServerChannel;

        cl::Channel::MessageReceivedCallback messageCallback =
            [&clientToServerChannel, &messagesReceived](
                cl::Message &message) {
                bool a = true;
                ASSERT_TRUE(message.decode(a));
                ASSERT_TRUE(a == false);

                int i = 0;
                ASSERT_TRUE(message.decode(i));
                ASSERT_TRUE(i == messagesReceived);
                messagesReceived++;

                char c = 'a';
                ASSERT_TRUE(message.decode(c));
                ASSERT_TRUE(c == 'c');

                int d = 0;
                ASSERT_FALSE(message.decode(d));

                if (messagesReceived == MessagesCount) {
                    clientToServerChannel->unscheduleFromLooper(
                        cl::Looper::Current());
                    cl::Looper::Current()->terminate();
                }
            };

        server.channelAvailabilityCallback(
            [&clientToServerChannel, &messageCallback](
                std::shared_ptr<cl::Channel> channel) {

                if (clientToServerChannel.get() != nullptr) {
                    /*
                     *  We only need one connection
                     */
                    channel->terminate();
                    return;
                }

                clientToServerChannel.swap(channel);
                clientToServerChannel->messageReceivedCallback(messageCallback);
                clientToServerChannel->scheduleInLooper(
                    cl::Looper::Current());
            });

        auto looper = cl::Looper::Current();
        looper->addSource(server.clientConnectionsSource());
        looper->loop();
    });

    std::thread clientThread([endpoint]() {
        cl::Channel channel(endpoint);
        ASSERT_TRUE(channel.isReady());

        for (int i = 0; i < 5; i++) {
            if (channel.tryConnect()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        ASSERT_TRUE(channel.isConnected());

        for (int i = 0; i < MessagesCount; i++) {
            cl::Message message;

            message.encode(false);
            message.encode(i);
            message.encode('c');

            ASSERT_TRUE(channel.sendMessage(message));
        }
    });

    clientThread.join();
    serverThread.join();

    ASSERT_TRUE(messagesReceived == MessagesCount);
}

TEST(ChannelTest, CreateConnectedChannels) {
    auto connectedChannels = cl::Channel::CreateConnectedChannels();
    ASSERT_TRUE(connectedChannels.first.get() != nullptr);
    ASSERT_TRUE(connectedChannels.second.get() != nullptr);
}
