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

#include <stdio.h>

#include "Looper.h"
#include <gtest/gtest.h>
#include <thread>

TEST(LooperTest, CurrentLooperAccess) {
    cl::Looper *looper = cl::Looper::Current();
    ASSERT_TRUE(looper != nullptr);
    ASSERT_TRUE(cl::Looper::Current() == looper);
}

TEST(LooperTest, LooperOnAnotherThread) {

    cl::Looper *looper1 = cl::Looper::Current();
    cl::Looper *looper2 = nullptr;

    std::thread thread([&] { looper2 = cl::Looper::Current(); });

    thread.join();

    ASSERT_TRUE(looper1 != looper2);
}

TEST(LooperTest, SimpleLoop) {
    std::thread thread([] {
        auto outer = cl::Looper::Current();

        bool terminatedFromInner = false;

        std::thread innerThread([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            terminatedFromInner = true;
            outer->terminate();
            ASSERT_TRUE(true);
        });

        /*
         *  Keep looping till the inner thread terminates
         *  the loop.
         */
        outer->loop();

        ASSERT_TRUE(terminatedFromInner);

        innerThread.join();
    });

    thread.join();
    ASSERT_TRUE(true);
}

TEST(LooperTest, Timer) {
    std::thread timerThread([] {

        auto looper = cl::Looper::Current();

        std::chrono::high_resolution_clock clock;

        auto start = clock.now();

        auto timer =
            cl::LooperSource::AsTimer(std::chrono::milliseconds(10));

        timer->setWakeFunction([clock, start]() {
            long long duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    clock.now() - start).count();
            cl::Looper::Current()->terminate();
            ASSERT_TRUE(
                duration >= 5 &&
                duration <=
                    15); /* FIXME: brittle, need a better way to check. */
        });

        looper->addSource(timer);

        looper->loop();

    });

    timerThread.join();
}

TEST(LooperTest, TimerRepetition) {

    int count = 0;

    std::thread timerThread([&count] {

        auto looper = cl::Looper::Current();

        auto timer =
            cl::LooperSource::AsTimer(std::chrono::milliseconds(1));

        timer->setWakeFunction([&count, looper]() {

            count++;

            if (count == 10) {
                looper->terminate();
            }
        });

        looper->addSource(timer);

        looper->loop();

    });

    timerThread.join();

    ASSERT_TRUE(count == 10);
}
