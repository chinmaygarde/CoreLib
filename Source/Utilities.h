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

#ifndef __CORELIB__UTILITIES__
#define __CORELIB__UTILITIES__

#include "Config.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>

#pragma mark - Assertions

#define CL_ASSERT assert

#pragma mark - Logging

#if CL_ENABLE_LOGS

#define CL_FILE_LAST_COMPONENT                                                 \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define CL_LOG_FMT "CoreLib (in '%s' @ %s:%d): "
#define CL_LOG_ARG __FUNCTION__, CL_FILE_LAST_COMPONENT, __LINE__

#define CL_LOG(message, ...)                                                   \
    printf(CL_LOG_FMT message "\n", CL_LOG_ARG, ##__VA_ARGS__)
#define CL_LOG_ERRNO() CL_LOG("%s (%d)", strerror(errno), errno)

#else

#define CL_LOG(message, ...)
#define CL_LOG_ERRNO()

#endif

#pragma mark - Error Checking

#define CL_CHECK_EXPECT(call, expected)                                        \
    {                                                                          \
        if ((call) != (expected)) {                                            \
            CL_LOG_ERRNO();                                                    \
            CL_ASSERT(0 && "Error in UNIX call");                              \
        }                                                                      \
    }

#define CL_CHECK(call) CL_CHECK_EXPECT(call, 0)

#pragma mark - POSIX Retry

/**
 *  Retries operation on `EINTR`. Just like `TEMP_FAILURE_RETRY` but available
 *  with non-GNU headers
 */
#define CL_TEMP_FAILURE_RETRY(exp)                                             \
    ({                                                                         \
        __typeof__(exp) _rc;                                                   \
        do {                                                                   \
            _rc = (exp);                                                       \
        } while (_rc == -1 && errno == EINTR);                                 \
        _rc;                                                                   \
    })

#define CL_TEMP_FAILURE_RETRY_AND_CHECK(exp)                                   \
    ({                                                                         \
        __typeof__(exp) _rc;                                                   \
        do {                                                                   \
            _rc = (exp);                                                       \
        } while (_rc == -1 && errno == EINTR);                                 \
        CL_CHECK_EXPECT(_rc, 0);                                               \
        _rc;                                                                   \
    })

#endif /* defined(__CORELIB__UTILITIES__) */
