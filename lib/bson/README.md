# MongoDB C Driver

This is then 10gen-supported MongoDB C driver. There are two goals for this driver.
The first is to provide a strict, default compilation option for ultimate portability,
no dependencies, and generic embeddability.

The second is to support more advanced, platform-specific features, like socket timeout,
by providing an interface for platform-specific modules.

Until the 1.0 release, this driver should be considered alpha. Keep in mind that the API will be in flux until then.

# Documentation

Documentation exists in the project's `docs` folder. You can read the latest
docs online at (http://api.mongodb.org/c/current/).

The docs are built using Sphinx and Doxygen. If you have these tools installed, then
you can build the docs with scons:

    scons docs

The html docs will appear in docs/html.

# Building

First check out the version you want to build. *Always build from a particular tag, since HEAD may be
a work in progress.* For example, to build version 0.6, run:

    git checkout v0.6

You can then build the driver with scons:

    scons

For more build options, see the docs.

## Running the tests
Make sure that you're running mongod on 127.0.0.1 on the default port (27017). The replica set
test assumes a replica set with at least three nodes running at 127.0.0.1 and starting at port
30000. Note that the driver does not recognize 'localhost' as a valid host name.

To compile and run the tests:

    scons test

# Error Handling
Most functions return MONGO_OK or BSON_OK on success and MONGO_ERROR or BSON_ERROR on failure.
Specific error codes and error strings are then stored in the `err` and `errstr` fields of the
`mongo` and `bson` objects. It is the client's responsibility to check for errors and handle
them appropriately.

# ISSUES

You can report bugs, request new features, and view this driver's roadmap
using [JIRA](http://jira.mongodb.org/browse/CDRIVER).

# CREDITS

* Gergely Nagy - Non-null-terminated string support.
* Josh Rotenberg - Initial Doxygen setup and a significant chunk of documentation.

# LICENSE

Unless otherwise specified in a source file, sources in this
repository are published under the terms of the Apache License version
2.0, a copy of which is in this repository as APACHE-2.0.txt.
