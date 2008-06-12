ASIO-README.txt

This document contains information to help you compile PortAudio with 
ASIO support. If you find any omissions or errors in this document 
please notify Ross Bencina <rossb@audiomulch.com>.


Building PortAudio with ASIO support
------------------------------------

To build PortAudio with ASIO support you need to compile and link with
pa_asio.c, and files from the ASIO SDK (see below), along with the common 
files from pa_common/ and platform specific files from pa_win/ (for Win32) 
or pa_mac/ (for Macintosh).

If you are compiling with a non-Microsoft compiler on windows, also 
compile and link with iasiothiscallresolver.cpp (see below for 
an explanation).

For some platforms (MingW, possibly Mac), you may simply
be able to type:

./configure --with-host_os=mingw --with-winapi=asio [--with-asiodir=/usr/local/asiosdk2]
make

./configure --with-host_os=darwin --with-winapi=asio [--with-asiodir=/usr/local/asiosdk2]
make

and life will be good.


Obtaining the ASIO SDK
----------------------

In order to build PortAudio with ASIO support, you need to download 
the ASIO SDK (version 2.0) from Steinberg. Steinberg makes the ASIO 
SDK available to anyone free of charge, however they do not permit its 
source code to be distributed.

NOTE: In some cases the ASIO SDK may require patching, see below 
for further details.

http://www.steinberg.net/en/ps/support/3rdparty/asio_sdk/

If the above link is broken search Google for:
"download steinberg ASIO SDK"



Building the ASIO SDK on Macintosh
----------------------------------

To build the ASIO SDK on Macintosh you need to compile and link with the 
following files from the ASIO SDK:

host/asiodrivers.cpp 
host/mac/asioshlib.cpp 
host/mac/codefragements.cpp



Building the ASIO SDK on Windows
--------------------------------

To build the ASIO SDK on Windows you need to compile and link with the 
following files from the ASIO SDK:

asio_sdk\common\asio.cpp
asio_sdk\host\asiodrivers.cpp
asio_sdk\host\pc\asiolist.cpp

You may also need to adjust your include paths to support inclusion of 
header files from the above directories.

The ASIO SDK depends on the following COM API functions: 
CoInitialize, CoUninitialize, CoCreateInstance, CLSIDFromString
For compilation with MinGW you will need to link with -lole32, for
Borland link with Import32.lib.



Non-Microsoft (MSVC) Compilers on Windows including Borland and GCC
-------------------------------------------------------------------

Steinberg did not specify a calling convention in the IASIO interface 
definition. This causes the Microsoft compiler to use the proprietary 
thiscall convention which is not compatible with other compilers, such 
as compilers from Borland (BCC and C++Builder) and GNU (gcc). 
Steinberg's ASIO SDK will compile but crash on initialization if 
compiled with a non-Microsoft compiler on Windows.

PortAudio solves this problem using the iasiothiscallresolver library 
which is included in the distribution. When building ASIO support for
non-Microsoft compilers, be sure to compile and link with
iasiothiscallresolver.cpp. Note that iasiothiscallresolver includes
conditional directives which cause it to have no effect if it is
compiled with a Microsoft compiler, or on the Macintosh.

If you use configure and make (see above), this should be handled
automatically for you.

For further information about the IASIO thiscall problem see this page:
http://www.audiomulch.com/~rossb/code/calliasio



Macintosh ASIO SDK Bug Patch
----------------------------

There is a bug in the ASIO SDK that causes the Macintosh version to 
often fail during initialization. Below is a patch that you can apply.

In codefragments.cpp replace getFrontProcessDirectory function with 
the following one (GetFrontProcess replaced by GetCurrentProcess).


bool CodeFragments::getFrontProcessDirectory(void *specs)
{
	FSSpec *fss = (FSSpec *)specs;
	ProcessInfoRec pif;
	ProcessSerialNumber psn;

	memset(&psn,0,(long)sizeof(ProcessSerialNumber));
	//  if(GetFrontProcess(&psn) == noErr)  // wrong !!!
	if(GetCurrentProcess(&psn) == noErr)  // correct !!!
	{
		pif.processName = 0;
		pif.processAppSpec = fss;
		pif.processInfoLength = sizeof(ProcessInfoRec);
		if(GetProcessInformation(&psn, &pif) == noErr)
				return true;
	}
	return false;
}


---
