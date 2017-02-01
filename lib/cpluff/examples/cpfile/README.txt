C-PLUFF FILE COMMAND EXAMPLE
============================

Overview
--------

On UNIX systems the file(1) utility can be used to determine file type and
to get information about contents of a file. Here are couple of examples
of file usage in a Linux environment.

  $ file /sbin/init
  /sbin/init: ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV),
  for GNU/Linux 2.4.1, dynamically linked (uses shared libs), for
  GNU/Linux 2.4.1, stripped
  
  $ file COPYRIGHT.txt
  COPYRIGHT.txt: ASCII English text

This example shows how a simplistic file clone could be implemented as an
extensible application based on C-Pluff. We will call the resulting
utility cpfile. It can recognize some special files and some file types
based on file extension. But it could be further extended to recognize
files based on their content by deploying a suitable plug-in. Notice that
the focus here was on creating a straightforward example rather than an
efficient one.


Architecture
------------

This example uses the generic plug-in loader, cpluff-loader, as the main
program. The executable cpfile installed into the bin directory is just
a shell script invoking the cpluff-loader. All program logic is included
in plug-ins.

The included plug-ins are:

  org.c-pluff.examples.cpfile.core

    This plug-in is the one initially started via cpluff-loader. It
    contains the core application logic and provides an extension point
    for file classifiers. The plug-in itself does not include any file
    classifiers. Instead it uses file classifiers registered as
    extensions by other plug-ins and then tries them one at a time in
    order of decreasing priority until a matching classification is
    found or no more classifiers are left.

  org.c-pluff.examples.cpfile.special

    This plug-in provides a file classifier which uses lstat(2) on the
    file to be classified to see if it is a special file such as a
    directory or a symbolic link. It also checks for the existence of
    the file.

  org.c-pluff.examples.cpfile.extension

    This plug-in provides a file classifier which checks the file name
    for known extensions. The plug-in provides an extension point for
    file extensions. The file extensions registered as extensions are
    then matched against the file name. The plug-in itself includes an
    extension for text files.

  org.c-pluff.examples.cpfile.cext

    This plug-in does not include a runtime library at all. Instead, it
    just registers some file types and file extensions related to
    C program source files.

Having build and installed the example, you can experiment with different
plug-in configurations by adding and removing plug-ins into cpfile/plugins
directory in the library directory. The core plug-in must be always
included for the application to work as intended.

You can create a new plug-in for the example by creating a new
subdirectory in the plugins source directory and adding it to SUBDIRS
variable in Makefile.am in the plugins source directory.


Example runs
------------

Here are couple of examples of using the resulting cpfile application.

  $ cpfile /tmp/testdir
  C-Pluff Loader, version 0.1.0
  C-Pluff Library, version 0.1.0 for i686-pc-linux-gnu
  /tmp/testdir: directory
  
  $ cpfile /tmp/test.foo
  C-Pluff Loader, version 0.1.0
  C-Pluff Library, version 0.1.0 for i686-pc-linux-gnu
  /tmp/test.foo: unknown file type
  
  $ cpfile /tmp/test.c
  C-Pluff Loader, version 0.1.0
  C-Pluff Library, version 0.1.0 for i686-pc-linux-gnu
  /tmp/test.c: C source file

  $ cpfile /tmp/test.nonexisting
  C-Pluff Loader, version 0.1.0
  C-Pluff Library, version 0.1.0 for i686-pc-linux-gnu
  /tmp/test.nonexisting: stat failed: No such file or directory

You can make cpfile more quiet by giving it -q option, or more verbose by
giving it -v option (repeated for more verbosity up to -vvv). Actually,
these options are processed by cpluff-loader which configures logging
accordingly.

  $ cpfile -q /tmp/test.c
  /tmp/test.c: C source file
  
  $ cpfile -vv /tmp/test.c
  C-Pluff Loader, version 0.1.0
  C-Pluff Library, version 0.1.0 for i686-pc-linux-gnu
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.core has been installed.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.extension has been installed.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.cext has been installed.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.special has been installed.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.core runtime has been loaded.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.core is starting.
  C-Pluff: INFO: [org.c-pluff.examples.cpfile.core] Plug-in org.c-pluff.examples.cpfile.extension runtime has been loaded.
  C-Pluff: INFO: [org.c-pluff.examples.cpfile.core] Plug-in org.c-pluff.examples.cpfile.extension is starting.
  C-Pluff: INFO: [org.c-pluff.examples.cpfile.core] Plug-in org.c-pluff.examples.cpfile.extension has been started.
  C-Pluff: INFO: [org.c-pluff.examples.cpfile.core] Plug-in org.c-pluff.examples.cpfile.special runtime has been loaded.
  C-Pluff: INFO: [org.c-pluff.examples.cpfile.core] Plug-in org.c-pluff.examples.cpfile.special has been started.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.core has been started.
  /tmp/test.c: C source file
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.core is stopping.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.core has been stopped.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.special has been stopped.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.extension has been stopped.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.core runtime has been unloaded.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.core has been uninstalled.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.extension runtime has been unloaded.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.extension has been uninstalled.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.cext has been uninstalled.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.special runtime has been unloaded.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.special has been uninstalled.
