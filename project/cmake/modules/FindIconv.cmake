# based on
# https://github.com/doxygen/doxygen/blob/master/cmake/FindIconv.cmake

# vim:ts=4:sw=4:expandtab:autoindent:
#
# The MIT License
#
# Copyright (c) 2008, 2009 Flusspferd contributors (see "CONTRIBUTORS" or
#                                      http://flusspferd.org/contributors.txt)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

Include(CheckFunctionExists)
include(CheckCXXSourceCompiles)

if(ICONV_INCLUDE_DIR)
  set(ICONV_FIND_QUIETLY TRUE)
endif()

find_path(ICONV_INCLUDE_DIR iconv.h
    HINTS
    ${CMAKE_PREFIX_PATH}
    ${ICONV_DIR}
    $ENV{ICONV_DIR}
    PATH_SUFFIXES include
)

if(NOT ICONV_INCLUDE_DIR STREQUAL "ICONV_INCLUDE_DIR-NOTFOUND")
    check_function_exists(iconv_open ICONV_IN_GLIBC)
endif()

if(NOT ICONV_IN_GLIBC)
    if (CMAKE_CL_64)
        find_library(ICONV_LIBRARY
            NAMES iconv64
            HINTS
            ${CMAKE_PREFIX_PATH}
            ${ICONV_DIR}
            $ENV{ICONV_DIR}
            PATH_SUFFIXES lib64 lib
            )
    else()
        find_library(ICONV_LIBRARY
            NAMES iconv
            HINTS
            ${CMAKE_PREFIX_PATH}
            ${ICONV_DIR}
            $ENV{ICONV_DIR}
            PATH_SUFFIXES lib64 lib
            )
    endif()
    set(ICONV_TEST ${ICONV_LIBRARY})
else()
    set(ICONV_TEST "In glibc")
endif()

set(CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARY})

check_cxx_source_compiles(
    "#include <iconv.h>
     int main() {
        iconv(iconv_t(-1), 0, 0, 0, 0);
     }"
    ICONV_COMPILES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ICONV DEFAULT_MSG ICONV_TEST ICONV_INCLUDE_DIR ICONV_COMPILES)

if(ICONV_FOUND)
    set(ICONV_LIBRARIES ${ICONV_LIBRARY})
    set(CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_DIR})
    set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARIES})

    if (NOT DEFINED ICONV_ACCEPTS_NONCONST_INPUT)
        # Display a useful message first time we come through here
        message(STATUS "One (and only one) of the ICONV_ACCEPTS_... tests must pass")
    endif()
    check_cxx_source_compiles(
        "#include <iconv.h>
         int main() {
            char *p = 0;
            iconv(iconv_t(-1), &p, 0, 0, 0);
         }"
        ICONV_ACCEPTS_NONCONST_INPUT)

    check_cxx_source_compiles(
        "#include <iconv.h>
         int main() {
            char const *p = 0;
            iconv(iconv_t(-1), &p, 0, 0, 0);
         }"
        ICONV_ACCEPTS_CONST_INPUT)

    if (ICONV_LIBRARY)
        list(REMOVE_ITEM CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARY})
        list(REMOVE_DUPLICATES CMAKE_REQUIRED_LIBRARIES)
    endif()
endif()

if(NOT ICONV_ACCEPTS_CONST_INPUT AND NOT ICONV_ACCEPTS_NONCONST_INPUT)
  MESSAGE(FATAL_ERROR "Unable to determine iconv() signature")
elseif(ICONV_ACCEPTS_CONST_INPUT AND ICONV_ACCEPTS_NONCONST_INPUT)
  MESSAGE(FATAL_ERROR "Unable to determine iconv() signature - both test cases passed!")
endif()

mark_as_advanced(ICONV_LIBRARY ICONV_INCLUDE_DIR)
