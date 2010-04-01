This comes in three parts:
1) Build a 'special' Qt for use with llqtwebkit
2) Build llqtwebkit
3) Get the resulting bits in the SL Viewer (if that's what you're interested in)


1) BUILDING Qt FOR LINUX LLQTWEBKIT
-----------------------------------

Acquire the Qt 4.6 source with our patches
 * Our clone of Qt is available here: http://gitorious.org/+lindenqt/qt/lindenqt
 * Download the source by clicking on the "lindenqt" branch under "Branches" and then using the "download as tar.gz" link on the right hand side of the page
     o Alternately, if you prefer you can clone the repository with git using the "clone" link on the page -- details of this are left as an exercise to the reader 
 * Extract the tarball into ~/llqtwebkit (or wherever else you've unpacked this source repository)
 * This may take some as the archive contains a lot of files
 * You should now have a directory ~/llqtwebkit/qt-lindenqt
 * Open a terminal window and cd to ~/llqtwebkit/qt-lindenqt directory
 * run the following commands (we'll use the QTDIR variable in later steps) 
      export QTDIR=`pwd`
      export PATH=$PATH:$QTDIR/bin


Configure the Build
$ CXXFLAGS='-DQT_NO_INOTIFY' ./configure -v -fontconfig -fast -no-qt3support -static -release  -no-xmlpatterns -no-phonon -openssl-linked -no-3dnow -no-sse -no-sse2 -no-gtkstyle -no-xinput -no-sm -buildkey LL`date +%s` -no-sql-sqlite -no-scripttools -no-cups -no-dbus -no-libmng -no-glib -qt-libpng -opengl desktop  -no-xkb -xrender -svg

* Are you making a build for redistribution to other people and you are not specifically on your distribution?  Then please add '-fno-stack-protector' to the CXXFLAGS above.  This is important!  Otherwise your resulting lib will not be usable at runtime on many machines.

* Accept the license, if you do.

* Wait a few minutes while it churns and bootstraps.

* Build Qt:
$ make

* Wait ages for it to build.

* You're done, but if you want to go on to build llqtwebkit on this machine, you'd really better do...

$ sudo make install


2) BUILDING LLQTWEBKIT
----------------------

* set up environment vars
$ export QTDIR=/usr/local/Trolltech/Qt-4.5.2
$ export PATH=$PATH:$QTDIR/bin

* configure llqtwebkit
$ qmake CONFIG-=debug

* Are you making a build for redistribution to other people and you are not specifically on your distribution?  Then please hack the resulting Makefile and add '-fno-stack-protector' to CXXFLAGS and CFLAGS.  This is important!  Otherwise your resulting lib will not be usable at runtime on many machines.

* build it
$ make


3) THE BITS THAT THE VIEWER BUILD NEEDS
---------------------------------------

* From Qt:
 lib/*.a
 plugins/imageformats/*.a

* From LLQtWebKit:
 libllqtwebkit.a
 llqtwebkit.h

The .a files go in {VIEWERSRC}/libraries/i686-linux/lib_release_client/
The .h files go in {VIEWERSRC}/libraries/include/

