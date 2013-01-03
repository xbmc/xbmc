##
# Copyright (c) 2008-2012 Marius Zwicker
# All rights reserved.
# 
# @LICENSE_HEADER_START:Apache@
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 
# http://www.mlba-team.de
# 
# @LICENSE_HEADER_END:Apache@
##

FIND_PATH(
	MICROHTTPD_INCLUDE_DIRS
	NAMES
	microhttpd.h
	HINTS
	/Library/Frameworks
	$ENV{HOME}/include
	/usr/local/include
	/usr/include
	/opt/local/include
)

set(CMAKE_FIND_FRAMEWORK LAST)

FIND_LIBRARY(
	MICROHTTPD_LIBRARY_RELEASE
	NAMES
	microhttpd libmicrohttpd
	HINTS
	/Library/Frameworks
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	/opt/local/lib	
)

IF (MICROHTTPD_LIBRARY_RELEASE)
	SET(MICROHTTPD_LIBRARIES ${MICROHTTPD_LIBRARY_RELEASE})
ENDIF ()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	microhttpd
	DEFAULT_MSG
	MICROHTTPD_INCLUDE_DIRS
	MICROHTTPD_LIBRARIES
)

MARK_AS_ADVANCED(
	MICROHTTPD_INCLUDE_DIRS
	MICROHTTPD_LIBRARIES
	MICROHTTPD_LIBRARY_RELEASE
) 
