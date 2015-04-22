# The MIT License (MIT)
# 
# Copyright (c) 2015, Chinmay Garde
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


cmake_minimum_required(VERSION 2.8)

################################
# Core Library
################################
project(CoreLib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_definitions ("-Wno-reorder")

file(GLOB CORELIB_SRC
    "Source/*.h"
    "Source/*.cpp"
)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    file(GLOB CORELIB_SRC_PLATFORM
        "Source/Apple/*.h"
        "Source/Apple/*.cpp"
    )
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    file(GLOB CORELIB_SRC_PLATFORM
        "Source/Linux/*.h"
        "Source/Linux/*.cpp"
    )
endif()

include_directories ("Headers")

add_library(CoreLib ${CORELIB_SRC} ${CORELIB_SRC_PLATFORM})

if(UNIX AND NOT APPLE)
    target_link_libraries(CoreLib rt)
endif()

################################
# Google Test
################################

add_subdirectory (Test/GoogleTest)
enable_testing ()
include_directories (${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

################################
# Unit Tests
################################

file(GLOB CORELIB_TEST_SRC
    "Test/*.h"
    "Test/*.cpp"
)

include_directories ( Source )
add_executable( CoreLibTest ${CORELIB_TEST_SRC} )
target_link_libraries( CoreLibTest CoreLib gtest gtest_main)
add_test( CoreLibTestAll CoreLibTest )
