README.TXT for the Appweb Server.
--------------------------------------------------------------------------------

The Appweb server directory builds and packages the Appweb product. The 
source in this directory builds the appweb main program.

You can run Appweb by hand or use "make run" to run it. To debug on linux,
type "make debug". To debug on Windows, run Visual Studio and open 
../bin/Debug/appweb.exe.

Appweb can be configured via the command line or it can parse a configuration
file of directives.

To run appweb using a configuration file:

    ../bin/appweb -f appweb.conf

To run appweb via the command line:

    ../bin/appweb -a IP:PORT -d webDirectory

To display trace to the console:

    ../bin/appweb -f appweb.conf -l stdout:2


Windows
-------
On windows, Appweb can run in three modes. It can run as a windows service, 
as a graphical windows application or as a command line application. By
default, it is installed as a windows service. Once Appweb is installed, you
can use the system services applet under Control Panel -> administration tools
to start and stop the Appweb service.

Appweb Command Line Options
---------------------------
    appweb [-Abdkmv] [-a IP:PORT] [-d docRoot] [-f configFile] \
        [-l logSpec] [-r serverRootDir] [-t numThreads]

    Options:\n");
      -a IP:PORT     Address to listen on
      -A             Auto-scan for a free port
      -b             Run in background as a daemon
      -c             Suppress the console display on windows
      -d docRoot     Web directory (DocumentRoot)
      -f configFile  Configuration file name
      -k             Kill existing running http
      -l file:level  Log to file at verbosity level
      -r serverRoot  Alternate Home directory

    Debug options
      -D             Debug mode (no timeouts)
      -m             Output memory stats
      -t number      Use number of pool threads
      -v             Output version information

    Windows Service options
      -i option      Install appweb as a service. Use "-i default" to install 
                     Appweb with the default options "-b -c -f appweb.conf".
      -g             Go (start) service
      -s             Stop service
      -u             Uninstall service

Other important files to read:

FILES.TXT -- List and description of files and directories.

--------------------------------------------------------------------------------

Copyright (c) 2003-2007 Mbedthis Software, LLC. All Rights Reserved.
Mbedthis and Appweb are trademarks of Mbedthis Software, LLC. Other 
brands and their products are trademarks of their respective holders.

See LICENSE.TXT for software license details.

--------------------------------------------------------------------------------
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim: sw=4 ts=4 
#
