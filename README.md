Core Library
============

C++ library containing useful classes, utility methods or anything else that
that counts as boilerplate when starting new C++ projects.

This library is a work in progress as usefulecomponents needs to be extracted 
from various personal projects

Components
----------

*  Events Loops
*  High Resolution Timers
*  IPC Mechanisms (Shared Memory, Messaging, FD Transfer, etc.)
*  The usual logging and assertion utilities

Usage
-----

* Add this repository as a submodule/subdirectory of the project using the same
* Add the following to the `CMakeLists.txt`:
```
add_subdirectory(CoreLib)
include_directories(${CoreLib_SOURCE_DIR}/Headers)
target_link_libraries(... CoreLib)
```
