Mbedthis Mbedthis Appweb(TM)
README.TXT

Welcome to the Mbedthis Appweb distribution. This document contains details 
regarding the installation procedure for the binary Mbedthis Appweb package.
This package contains a pre-built stand-alone (binary) web server and the 
development libraries and headers needed to embed Appweb in an application.

Once installed, you may read the Appweb online documentation and samples for
further information. You can also read the documentation for the latest
release online at:

    http://www.mbedthis.com/products/appweb/doc/index.html


This software is copyrighted and distributed under license. Please read the 
LICENSE.TXT for details.


Table of Contents
-----------------

* System Requirements
* Linux Release Details
* Windows Release Details
* VxWorks Release Details
* Building the Source Code
* License and Copyright Information

System Requirements
-------------------

Operating System Support

    * Linux 2.4, 2.6 (x86,arm,ppc,sparc,xscale,mips)
        including redhat and debian based distributions.
    * Windows Vista, XP, 2000 (x86)
    * VxWorks (x86,arm,ppc,sparc,xscale,mips)
    * Mac OS X

Run-time System Requirements
----------------------------

To run Mbedthis Appweb, your system will need at least the following

    * 1MB RAM
    * 600 KB disk space
    * TCP/IP network stack

To develop applications that use Mbedthis Appweb you will need an additional

    * 15 MB disk space

To rebuild Appweb from source code, you will need at least

    * 30 MB disk space


Development Environment Platform Support
----------------------------------------

If you wish to embed Appweb in your application, build the samples or rebuild 
Mbedthis Appweb from source code, you will need to use a C/C++ compiler and 
associated development tools. Several development environments are supported. 
You may choose any of the following:

    * Linux GNU tools

    * Windows Visual Studio (Visual Studio 6 or Windows Visual Studio .NET 2005)
      This also requires the Cygwin UNIX emulation tools. See www.cygwin.com.

    * VxWorks Tornado II or later


Linux Release Details
---------------------

Installing Mbedthis Appweb

The installation distribution includes sub-packages containing pre-built 
binaries, development libraries, headers, documentation and samples. You can 
selectively install the sub-packages or you can install the complete 
distribution.

For those looking to build from source code, you will need to download the 
separate source code distribution. The product documentation describes how 
to build from source. 

The Mbedthis Appweb install distribution for LINUX is published in three 
formats:

    * Debian (*.deb) format for debian distributions including Ubuntu

    * RPM (*.rpm) format for RPM distributions including Fedora and RHEL

    * Tar format for custom installs.

It is recommended that you use the RPM or Debian formats if your system 
supports those package standards.

    * The Debian package is: appweb-VERSION-NUMBER-LINUX-i386.deb.tar.gz

    * The RPM package is: appweb-VERSION-NUMBER-LINUX-i386.rpm.tar.gz

    * The tar install package is: appweb-VERSION-NUMBER-LINUX-i386.tar.tar.gz

The RPM and Debian formats will register the software installation with the
system's package manager so that you can query, verify, and easily manage the
software on your system. 

The tar format consists of tar archives that you can manually extract for
maximum control over the placement of the product files. You can however,
still use the install script with the tar install distribution if 
your system does not support RPM.

To install the Linux RPM package:

   1. If you are installing the binary package, you will need to login as 
       root or attain root privilege when you execute the install script.

   2. Uncompress and extract the top level files by typing

      tar xvfz appweb-VERSION-NUMBER-LINUX-i386.rpm.tar.gz

   3. ./install

   4. If required, edit /etc/appweb.conf to suit your environment and then 
      restart Appweb via:

      service appweb restart


To install the Linux Debian package:

   1. If you are installing the binary package, you will need to login as 
      root or attain root privilege via sudo when you execute the install
      script.

   2. Uncompress and extract the top level files by typing

      tar xvfz appweb-VERSION-NUMBER-LINUX-i386.deb.tar.gz

   3. Install the package via:

      dpkg -i appweb-VERSION-NUMBER-LINUX-i386.deb

   4. If required, edit /etc/appweb.conf to suit your environment and then 
      restart Appweb via:

      invoke-rc.d appweb restart


To install the Linux Tar Package:

   1. If you are installing the binary package, you will need to login as root 
      or attain root privilege when you execute the install script.

   2. tar xvfz appweb-VERSION-NUMBER-LINUX-i386.tar.tar.gz

   3. cd {Desired-Directory-For-Binaries}

   4. tar xvfz appweb-VERSION-NUMBER-LINUX-i386.tar.gz


Removing Mbedthis Appweb

   1. Login as root

   2. Change directory to the installation directory for Appweb. This is 
      usually /etc/appweb

   3. Run the remove script in the install directory for Appweb by typing 

      ./remove

      or use your system removal tool. For RPM:

      rpm -e appweb

      or Debian:

      dpkg -r appweb


Running Appweb on Linux

The Appweb product installation will automatically start Appweb before it 
completes. Thereafter, Appweb will automatically start if the system is 
rebooted.

If you wish to stop Appweb, on Red Hat type:

    service appweb stop

    or on Debian, type:

    invoke-rc.d appweb stop

You can restart Appweb on Red Hat by:

    service appweb start

    or on Debian:

    invoke-rc.d appweb start

Alternatively, you may also run Appweb manually (once you have stopped the 
Appweb service as described above)

   1. cd /etc/appweb

   2. ./appweb -f appweb.conf -l stdout:2

Appweb is configured by default to listen on port 7777. To test that it is 
serving pages, point your browser at http://localhost:7777. To tailor the 
Mbedthis Appweb configuration, including the default port, you may wish to 
edit the appweb.conf configuration file. The -l switch directs logging 
output to the standard output (stdout) at level 2. Levels range between 0
and 9 with zero being the least verbose.


Windows Release Details
-----------------------

The installation distribution includes sub-packages containing pre-built 
binaries, development libraries and headers, documentation and samples. 
You can selectively install the sub-packages or you can install the complete 
distribution.

Installing Mbedthis Appweb

The Mbedthis Appweb Server install distribution for Windows is published in 
one format:

    * A ZIPped Windows installer executable named:

      appweb-VERSION-BUILD-WIN-i386.exe.zip

The package contains the binary distribution, libraries, headers, documentation 
and samples distribution in the one package. During installation you will have 
the option of installing the full package or just the binary or documentation 
and samples.

To install the Windows Installer package

   1. Login with administrator privileges. This is necessary as the binary 
      installation needs to install a Windows Service.

   2. Use WinZip or equivalent software to extract the README.TXT and 
      setup.exe from the zip archive.

   3. Select Run from the Start menu and browse to the location holding the 
      setup.exe install package and select it.

   4. Click OK.


Removing Mbedthis Appweb

If you installed the Tar package, you will need to remove Appweb manually. If 
you installed the Windows Installer package, follow these steps to remove
Appweb.

   1. Login with administrator privileges

   2. Run the Add / Remove Programs applet from the control panel.

   3. Select the Mbedthis Appweb Server and choose remove.


Running Appweb on Windows

Appweb can run in the background as a windows service, or in the foreground 
as a windows application or as a console application. 

The Appweb Windows Installer product installation will automatically start 
appweb as a Windows Service before the installation completes. Thereafter, 
Appweb will automatically start if the system is rebooted. You may start and 
stop Appweb manually if you wish. To do this, you will first need to disable 
the Appweb Windows Service via the following steps:

   1. Start the Windows Service Control Applet by right clicking on My 
      Computer and then selecting Manage. 

   2. Then click on Services and Applications in the left-hand pane. 

   3. Select Services in the left pane and then scroll in the right pane to 
      find Mbedthis Appweb and double click on Mbedthis Appweb.

   4. Select Start, Stop, Pause or Resume as required.

   5. Or change the Startup Type to be manual to permanently disable Appweb 
      as a Windows Service.


To Manually run Appweb as a windows app

   1. Ensure that appweb is not running as a service (as described above)

   2. Display the contents of Programs > Mbedthis Appweb under the Start menu.

   3. Select Mbedthis Appweb. The application will be started and its icon 
      will appear in the status area of the Windows taskbar. This is actually 
      starting appweb. You can double click on the appweb icon to launch
      a browser.


To Run Appweb from the Console

   1. Select Run from the Start menu and browse to the location holding the 
      Mbedthis Appweb installation. Browse to the App web program which by
      default is in C:/Program Files/Mbedthis Appweb/bin/appweb.exe. Select 
      the appweb.exe program.

   2. Add -f appweb.conf to the end of the command line

   3. Click OK

   4. A command.com window will appear

   5. To exit the application, close the window


Appweb is run as configured by default to startup up automatically as a system 
service and to listen on port 7777. To test that it is serving pages, point 
your browser at http://localhost:7777. To tailor the Mbedthis Appweb 
configuration, including the default port, you may wish to edit the 
appweb.conf configuration file.


Building the Source Code
------------------------

If you wish to rebuild the Appweb product from the distributed source code, 
please download the source code distribution from the Mbedthis web site. Then
read "Building from Source" in the Appweb product documentation for an 
explanation of the necessary steps to successfully rebuild the product.


Exporting
---------
This distribution includes cryptographic software.  The country in which you 
currently reside may have restrictions on the import, possession, use, 
and/or re-export to another country, of encryption software. BEFORE using 
any encryption software, please check your country's laws, regulations and 
policies concerning the import, possession, or use, and re-export of encryption 
software, to see if this is permitted.  See <http://www.wassenaar.org/> for more
information.

The U.S. Government Department of Commerce, Bureau of Industry and Security 
(BIS), has classified this software as Export Commodity Control Number (ECCN) 
5D002.C.1, which includes information security software using or performing 
cryptographic functions with asymmetric algorithms. This release is eligible 
for export under the License Exception ENC Technology Software Unrestricted 
(TSU) exception (see the BIS Export Administration Regulations, Section 740.13) 
for both object code and source code.


License and Copyright Information
---------------------------------

This software is supplied under license. Both a GNU and commercial license are 
available. Please read LICENSE.TXT for details. This documentation and the 
software described in this document may be used and copied only in accordance 
with the terms of the accompanying license agreement. Product and technical 
information in this document is subject to change without notice and does not 
represent a commitment on the part of Mbedthis Software LLC.

Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.

Mbedthis is a trademark of Mbedthis Software LLC. Mbedthis, Appweb, ESP and 
EGI are trademarks of Mbedthis Software. All other brand or product names are 
the trademarks or registered trademarks of their respective holders.


Mbedthis Software, LLC.
4616 25th Ave NE 
#733
Seattle, WA 98105.
+1 (425) 329-3490
www.mbedthis.com
info@mbedthis.com
