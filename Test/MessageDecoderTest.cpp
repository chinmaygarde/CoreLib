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
#include <gtest/gtest.h>

#include "Message.h"

TEST(MessageDecoderTest, SimpleInitialization) {
    cl::Message message;

    uint32_t val = 40;

    message.encode(val);

    val = 222;

    cl::Message decoder(message.data(), message.size());

    ASSERT_TRUE(decoder.decode(val));
    ASSERT_TRUE(val == 40);
}

TEST(MessageDecoderTest, MutltipleInitialization) {
    cl::Message message;

    uint32_t val = 40;
    bool foo = true;
    float a = 3.0f;
    double b = 20.0;
    char c = 'c';

    message.encode(val);
    message.encode(foo);
    message.encode(a);
    message.encode(b);
    message.encode(c);

    val = 0;
    foo = false;
    a = 0.0f;
    b = 0.0;
    c = 'd';

    cl::Message decoder(message.data(), message.size());

    ASSERT_TRUE(decoder.decode(val));
    ASSERT_TRUE(decoder.decode(foo));
    ASSERT_TRUE(decoder.decode(a));
    ASSERT_TRUE(decoder.decode(b));
    ASSERT_TRUE(decoder.decode(c));

    ASSERT_FALSE(decoder.decode(val));

    ASSERT_TRUE(val == 40);
    ASSERT_TRUE(foo == true);
    ASSERT_TRUE(a == 3.0f);
    ASSERT_TRUE(b == 20.0);
    ASSERT_TRUE(c == 'c');
}
