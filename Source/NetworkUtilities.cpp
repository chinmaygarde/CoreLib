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


#include "NetworkUtilities.h"
#include "Utilities.h"

#include <mutex>
#include <sys/socket.h>
#include <unistd.h>

using namespace cl;
using namespace cl::NetworkUtil;

bool cl::NetworkUtil::SocketSeqPacketSupported() {
    static std::once_flag once;
    static bool supported = false;

    std::call_once(once, []() {
        /*
         *  Try to create SOCK_SEQPACKET socket and check result
         */
        int handle = socket(AF_UNIX, SOCK_SEQPACKET, 0);

        supported = (handle != -1);

        if (supported) {
            CL_CHECK(::close(handle));
        }
    });

    return supported;
}

int cl::NetworkUtil::SupportedDomainSocketType() {
    return SocketSeqPacketSupported() ? SOCK_SEQPACKET : SOCK_STREAM;
}
