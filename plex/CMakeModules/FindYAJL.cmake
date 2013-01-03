# @@@LICENSE
#
#      Copyright (c) 2012 Hewlett-Packard Development Company, L.P.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License looks.
#
# LICENSE@@@

# This will define:
# YAJL_FOUND - system has yajl
# YAJL_INCLUDE_DIRS - include directories necessary to compile w/ yajl
# YAJL_LIBRARIES - libraries necessary to link to to get yajl
#
# If YAJL_STATIC is set, find the static library; otherwise, find the shared library.
#

include(FindPackageHandleStandardArgs)

# Find the include directories
find_path(YAJL_INCLUDE_DIRS NAMES yajl/yajl_parse.h yajl/yajl_gen.h yajl/yajl_common.h)
find_path(YAJL_VERSION_DIR NAMES yajl/yajl_version.h)
if(YAJL_VERSION_DIR)
 	set(HAVE_YAJL_YAJL_VERSION_H 1)
endif()

# Find the library
if(YAJL_STATIC)
	find_library(YAJL_LIBRARIES NAMES yajl_s)
else()
	find_library(YAJL_LIBRARIES NAMES yajl)
endif()

find_package_handle_standard_args(YAJL DEFAULT_MSG YAJL_INCLUDE_DIRS YAJL_LIBRARIES)

if(YAJL_FOUND)
	mark_as_advanced(YAJL_INCLUDE_DIRS YAJL_LIBRARIES)
else()
	unset(YAJL_INCLUDE_DIRS)
	unset(YAJL_LIBRARIES)
endif()
