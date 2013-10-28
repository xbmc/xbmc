TinyThread++ v1.0
=================

http://tinythread.sourceforge.net


About
-----

TinyThread++ is a minimalist, portable, threading library for C++, intended to
make it easy to create multi threaded C++ applications.

The library is closesly modeled after the current C++0x standard (draft), but
only a subset is implemented at the moment.

See the documentation in the doc/html directory for more information.


Using TinyThread++
------------------

To use TinyThread++ in your own project, just add tinythread.cpp and
tinythread.h to your project. In your own code, do:

#include <tinythread.h>
using namespace tthread;

If you wish to use the fast_mutex class, inlude fast_mutex.h:

#include <fast_mutex.h>


Building the test programs
--------------------------

From the test folder, issue one of the following commands:

Linux, Mac OS X, OpenSolaris etc:
  make   (you may need to use gmake on some systems)

Windows/MinGW:
  mingw32-make

Windows/MS Visual Studio:
  nmake /f Makefile.msvc


History
-------

v1.0 - 2010.10.01
  - First non-beta release.
  - Made mutex non-recursive (according to spec), and added recursive_mutex.
  - General class, code & documentation improvements.
  - Added a Makefile for MS Visual Studio.

v0.9 - 2010.08.10
  - Added preliminary support for this_thread::sleep_for().

v0.8 - 2010.07.02
  - Switched from CreateThread() to _beginthreadex() for Win32 (should fix
    tiny memory leaks).
  - Better standards compliance and some code cleanup.

v0.7 - 2010.05.17
  - Added this_thread::yield().
  - Replaced the non-standard number_of_processors() function with
    thread::hardware_concurrency(), which is part of the C++0x draft.
  - The thread::id() class is now more standards compliant (correct namespace
    and comparison operators).

v0.6 - 2010.04.28
  - Added a fast_mutex class (in fast_mutex.h).
  - Made the test.cpp application compile under Mac OS X and MinGW/g++ 3.x.

v0.5 - 2010.03.31
  - Added the thread_local keyword (support for thread-local storage).
  - Added a test application to test the API (test.cpp).
  - Improved the Doxygen documentation.

v0.4 - 2010.03.27
  - Added thread::get_id() and this_thread::get_id().
  - Changed the namespace name from tinythread to tthread.

v0.3 - 2010.03.24
  - Fixed a compiler error for fractal.cpp under MS Visual C++.
  - Added colors to the fractal generator.

v0.2 - 2010.03.23
  - Better C++0x conformance.
  - Better documentation.
  - New classes:
    - lock_guard
  - New member functions:
    - thread::joinable()
    - thread::native_handle()
    - mutex::try_lock()
  - Added a multi threaded fractal generator test application.

v0.1 - 2010.03.21
  - Initial release.


License
-------

Copyright (c) 2010 Marcus Geelnard

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
