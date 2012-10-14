Building the MongoDB C Driver
=============================

First checkout the version you want to build. *Always build from a particular tag, since HEAD may be
a work in progress.* For example, to build version 0.5.1, run:

.. code-block:: bash

    git checkout v0.5.1

Then follow the build steps below.

Compile options with custom defines
----------------------------------

Before compiling, you should note the following compile options.

For big-endian support, define:

- ``MONGO_BIG_ENDIAN``

If your compiler has a plain ``bool`` type, define:

- ``MONGO_HAVE_BOOL``

Alternatively, if you must include ``stdbool.h`` to get ``bool``, define:

- ``MONGO_HAVE_STDBOOL``

If you're not using C99, then you must choose your 64-bit integer type by
defining one of these:

- ``MONGO_HAVE_STDINT`` - Define this if you have ``<stdint.h>`` for int64_t.
- ``MONGO_HAVE_UNISTD`` - Define this if you have ``<unistd.h>`` for int64_t.
- ``MONGO_USE__INT64``  - Define this if ``__int64`` is your compiler's 64bit type (MSVC).
- ``MONGO_USE_LONG_LONG_INT`` - Define this if ``long long int`` is your compiler's 64-bit type.

Building with Make:
-------------------

If you're building the driver on posix-compliant platforms, including on OS X
and Linux, then you can build with ``make``.

To compile the driver, run:

.. code-block:: bash

    make

This will build the following libraries:

* libbson.a
* libbson.so (libbson.dylib)
* libmongoc.a
* lobmongoc.so (libmongoc.dylib)

You can install the libraries with make as well:

.. code-block:: bash

    make install

And you can run the tests:

.. code-block:: bash

    make test

You can even build the docs:

.. code-block:: bash

    make docs

By default, ``make`` will build the project in ``c99`` mode. If you want to change the
language standard, set the value of STD. For example, if you want to build using
the ANSI C standard, set STD to c89:

.. code-block:: bash

    make STD=c89

Once you've built and installed the libraries, you can compile the sample
with ``gcc`` like so:

.. code-block:: bash

    gcc --std=c99 -I/usr/local/include -L/usr/local/lib -o example docs/examples/example.c -lmongoc

If you want to statically link the program, add the ``-static`` option:

.. code-block:: bash

    gcc --std=c99 -static -I/usr/local/include -L/usr/local/lib -o example docs/examples/example.c -lmongoc

Then run the program:

.. code-block:: bash

    ./example

Building with SCons:
--------------------

You may also build the driver using the Python build utility, SCons_.
This is required if you're building on Windows. Make sure you've
installed SCons, and then from the project root, enter:

.. _SCons: http://www.scons.org/

.. code-block:: bash

    scons

This will build static and dynamic libraries for both ``BSON`` and for the
the driver as a complete package. It's recommended that you build in C99 mode
with optimizations enabled:

.. code-block:: bash

    scons --c99

Once you've built the libraries, you can compile a program with ``gcc`` like so:

.. code-block:: bash

    gcc --std=c99 -static -Isrc -o example docs/example/example.c libmongoc.a

On Posix systems, you may also install the libraries with scons:

.. code-block:: bash

    scons install

To build the docs:

.. code-block:: bash

    scons docs

Building on Windows
-------------------

When building the driver on Windows, you must use the Python build
utility, SCons_. For your compiler, we recommend that you use Visual Studio.
If you don't have Visual Studio, a free version is available. Search for Visual
Studio C++ Express to find it.

If you're running on 32-bit Windows, you must compile the driver in 32-bit mode:

.. code-block:: bash

    scons --m32

If getaddrinfo and friends aren't available on your version of Windows, you may
compile without these features like so:

.. code-block:: bash

    scons --m32 --standard-env

Platform-specific features
--------------------------

The original goal of the MongoDB C driver was to provide a very basic library
capable of being embedded anywhere. This goal is now evolving somewhat given
the increased use of the driver. In particular, it now makes sense to provide
platform-specific features, such as socket timeouts and DNS resolution, and to
return platform-specific error codes.

To that end, we've organized all platform-specific code in the following files:

* ``env_standard.c``: a standard, platform-agnostic implementation.
* ``env_posix.c``: an implementation geared for Posix-compliant systems (Linux, OS X).
* ``env_win32.c``: a Windows implementation.

Each of these implements the interface defined in ``env.h``.

When building with ``make``, we use ``env_posix.c``. When building with SCons_, we
use ``env_posix.c`` or ``env_win32.c``, depending on the platform.

If you want to compile with the generic, platform implementation, you have to do so
explicity. In SCons_:

.. code-block:: bash

    scons --standard-env

Using ``make``:

.. code-block:: bash

    make ENV=standard

Dependencies
------------

The driver itself has no dependencies, but one of the tests shows how to create a JSON-to-BSON
converter. For that test to run, you'll need JSON-C_.

.. _JSON-C: http://oss.metaparadigm.com/json-c/

Test suite
----------

Make sure that you're running mongod on 127.0.0.1 on the default port (27017). The replica set
test assumes a replica set with at least three nodes running at 127.0.0.1 and starting at port
30000. Note that the driver does not recognize 'localhost' as a valid host name.

With make:

.. code-block:: bash

    make test

To compile and run the tests with SCons:

.. code-block:: bash

    scons test

You may optionally specify a remote server:

.. code-block:: bash

    scons test --test-server=123.4.5.67

You may also specify an alternate starting port for the replica set members:

.. code-block:: bash

    scons test --test-server=123.4.5.67 --seed-start-port=40000

