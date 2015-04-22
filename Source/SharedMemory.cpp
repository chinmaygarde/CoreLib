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

#include "SharedMemory.h"
#include "Utilities.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>

using namespace cl;

static const int SharedMemoryTempHandleMaxRetries = 25;

static std::string SharedMemory_RandomFileName() {
    auto random = rand() % RAND_MAX;

    std::stringstream stream;

    stream << "/CoreLib_SharedMemory" << random;

    return stream.str();
}

static Handle SharedMemory_CreateBackingIfNecessary(Handle handle) {

    /*
     *  A valid handle is already present. Nothing to do.
     */
    if (handle != -1) {
        return handle;
    }

    Handle newHandle = -1;

    auto tempFile = SharedMemory_RandomFileName();

    int tries = 0;

    while (tries++ < SharedMemoryTempHandleMaxRetries) {

        newHandle = ::shm_open(tempFile.c_str(),
                               O_RDWR  | O_CREAT | O_EXCL,
                               S_IRUSR | S_IWUSR);

        if (newHandle == -1 && errno == EEXIST) {
            /*
             *  The current handle already exists (the O_CREAT | O_EXCL
             *  check is atomic). Try a new file name.
             */
            tempFile = SharedMemory_RandomFileName();
            continue;
        }

        break;
    }

    /*
     *  We already have a file reference, unlink the
     *  reference by name.
     */
    if (newHandle != -1) {
        CL_CHECK(::shm_unlink(tempFile.c_str()));
    }

    return newHandle;
}

SharedMemory::SharedMemory(size_t size)
    : _ready(false), _address(nullptr), _handle(-1), _size(0) {

    /*
     *  Step 1: Create a handle to shared memory
     */
    _handle = SharedMemory_CreateBackingIfNecessary(-1);

    if (_handle == -1) {
        goto failure;
    }

    /*
     *  Step 2: Set the size of the shared memory
     */

    if (CL_TEMP_FAILURE_RETRY(::ftruncate(_handle, size)) == -1) {
        goto failure;
    }

    _size = size;

    /*
     *  Step 3: Memory map a region with the given handle
     */

    _address =
        ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, _handle, 0);

    if (_address == MAP_FAILED) {
        goto failure;
    }

    /*
     *  Jobs done!
     */

    _ready = true;

    return;

failure:
    CL_LOG_ERRNO();

    _ready = false;

    if (_address != nullptr) {
        CL_CHECK(::munmap(_address, size));
    }

    _size = 0;
    _address = nullptr;

    if (_handle == -1) {
        CL_CHECK(::close(_handle));
    }

    _handle = -1;
}

void SharedMemory::cleanup() {
    if (!_ready) {
        return;
    }

    CL_CHECK(::munmap(_address, _size));
    CL_CHECK(::close(_handle));

    _handle = -1;
    _size = 0;
    _address = nullptr;
    _ready = false;
}

SharedMemory::~SharedMemory() {
    cleanup();
}
