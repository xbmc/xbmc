#
#	make.java -- Java configuration for inclusion by makefiles
#
#	TODO - 	This is a temporary place to store various Java configuration settings
#			and paths. Should be moved back into configure and tools.config.
#
################################################################################
#
#	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#	The latest version of this code is available at http://www.mbedthis.com
#
#	This software is open source; you can redistribute it and/or modify it 
#	under the terms of the GNU General Public License as published by the 
#	Free Software Foundation; either version 2 of the License, or (at your 
#	option) any later version.
#
#	This program is distributed WITHOUT ANY WARRANTY; without even the 
#	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
#	See the GNU General Public License for more details at:
#	http://www.mbedthis.com/downloads/gplLicense.html
#	
#	This General Public License does NOT permit incorporating this software 
#	into proprietary programs. If you are unable to comply with the GPL, a 
#	commercial license for this software and support services are available
#	from Mbedthis Software at http://www.mbedthis.com
#
################################################################################
#
#	Java source version
#
JAVA_VERSION	= -source 1.4

#
#	WTK CLDC version
#
CLDC			= 11

#
#	WTK MIDP version
#
MIDP			= 20

###############################################################################
#
#	Java configuration
#

ifneq	($(BLD_WTK),)
	WTK			= $(shell cygpath -m $(BLD_WTK))
	BLD_J2ME	= 1
	__JARS		+= $(WTK)/lib/cldcapi$(CLDC).jar;$(WTK)/lib/midpapi$(MIDP).jar;$(WTK)/lib/mmapi.jar;$(WTK)/lib/wma11.jar;$(MAKE_JARS);$(JARS)
	CLASSPATH	= "$(__JARS);$(WTK)/lib;$(BLD_TOP)/java;$(BLD_TOP)/lib;$(BLD_TOP)/obj/classes"
	JFLAGS		= -bootclasspath "$(WTK)/lib/cldcapi$(CLDC).jar" \
				  -classpath $(CLASSPATH) -sourcepath "$(BLD_TOP)/java"
else
	CLASSPATH	=
	JFLAGS		= -classpath $(CLASSPATH) -sourcepath "$(BLD_TOP)/java"
endif

