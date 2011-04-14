# CMakeLists.txt
#
#
# This file allows building PCRE with the CMake configuration and build
# tool. Download CMake in source or binary form from http://www.cmake.org/
#
# Original listfile by Christian Ehrlicher <Ch.Ehrlicher@gmx.de>
# Refined and expanded by Daniel Richard G. <skunk@iSKUNK.ORG>
# 2007-09-14 mod by Sheri so 7.4 supported configuration options can be entered
# 2007-09-19 Adjusted by PH to retain previous default settings
# 2007-12-26 (a) On UNIX, use names libpcre instead of just pcre
#            (b) Ensure pcretest and pcregrep link with the local library,
#                not a previously-installed one.
#            (c) Add PCRE_SUPPORT_LIBREADLINE, PCRE_SUPPORT_LIBZ, and
#                PCRE_SUPPORT_LIBBZ2.
# 2008-01-20 Brought up to date to include several new features by Christian
#            Ehrlicher.
# 2008-01-22 Sheri added options for backward compatibility of library names
#            when building with minGW:
#            if "ON", NON_STANDARD_LIB_PREFIX causes shared libraries to
#            be built without "lib" as prefix. (The libraries will be named
#            pcre.dll, pcreposix.dll and pcrecpp.dll).
#            if "ON", NON_STANDARD_LIB_SUFFIX causes shared libraries to
#            be built with suffix of "-0.dll". (The libraries will be named
#            libpcre-0.dll, libpcreposix-0.dll and libpcrecpp-0.dll - same names
#            built by default with Configure and Make.
# 2008-01-23 PH removed the automatic build of pcredemo.
# 2008-04-22 PH modified READLINE support so it finds NCURSES when needed.
# 2008-07-03 PH updated for revised UCP property support (change of files)
# 2009-03-23 PH applied Steven Van Ingelgem's patch to change the name
#            CMAKE_BINARY_DIR to PROJECT_BINARY_DIR so that it works when PCRE
#            is included within another project.
# 2009-03-23 PH applied a modified version of Steven Van Ingelgem's patches to
#            add options to stop the building of pcregrep and the tests, and
#            to disable the final configuration report.
# 2009-04-11 PH applied Christian Ehrlicher's patch to show compiler flags that
#            are set by specifying a release type.

PROJECT(PCRE C CXX)

CMAKE_MINIMUM_REQUIRED(VERSION 2.4.6)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake) # for FindReadline.cmake

# external packages
FIND_PACKAGE( BZip2 )
FIND_PACKAGE( ZLIB )
FIND_PACKAGE( Readline )

# Configuration checks

INCLUDE(CheckIncludeFile)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckTypeSize)

CHECK_INCLUDE_FILE(dirent.h     HAVE_DIRENT_H)
CHECK_INCLUDE_FILE(sys/stat.h   HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE(sys/types.h  HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE(unistd.h     HAVE_UNISTD_H)
CHECK_INCLUDE_FILE(windows.h    HAVE_WINDOWS_H)

CHECK_INCLUDE_FILE_CXX(type_traits.h            HAVE_TYPE_TRAITS_H)
CHECK_INCLUDE_FILE_CXX(bits/type_traits.h       HAVE_BITS_TYPE_TRAITS_H)

CHECK_FUNCTION_EXISTS(bcopy     HAVE_BCOPY)
CHECK_FUNCTION_EXISTS(memmove   HAVE_MEMMOVE)
CHECK_FUNCTION_EXISTS(strerror  HAVE_STRERROR)
CHECK_FUNCTION_EXISTS(strtoll   HAVE_STRTOLL)
CHECK_FUNCTION_EXISTS(strtoq    HAVE_STRTOQ)
CHECK_FUNCTION_EXISTS(_strtoi64 HAVE__STRTOI64)

CHECK_TYPE_SIZE("long long"             LONG_LONG)
CHECK_TYPE_SIZE("unsigned long long"    UNSIGNED_LONG_LONG)

# User-configurable options
#
# (Note: CMakeSetup displays these in alphabetical order, regardless of
# the order we use here)

SET(BUILD_SHARED_LIBS OFF CACHE BOOL
    "Build shared libraries instead of static ones.")

OPTION(PCRE_BUILD_PCRECPP "Build the PCRE C++ library (pcrecpp)." ON)

SET(PCRE_EBCDIC OFF CACHE BOOL
    "Use EBCDIC coding instead of ASCII. (This is rarely used outside of mainframe systems)")

SET(PCRE_LINK_SIZE "2" CACHE STRING
    "Internal link size (2, 3 or 4 allowed). See LINK_SIZE in config.h.in for details.")

SET(PCRE_MATCH_LIMIT "10000000" CACHE STRING
    "Default limit on internal looping. See MATCH_LIMIT in config.h.in for details.")

SET(PCRE_MATCH_LIMIT_RECURSION "MATCH_LIMIT" CACHE STRING
    "Default limit on internal recursion. See MATCH_LIMIT_RECURSION in config.h.in for details.")

SET(PCRE_NEWLINE "LF" CACHE STRING
    "What to recognize as a newline (one of CR, LF, CRLF, ANY, ANYCRLF).")

SET(PCRE_NO_RECURSE OFF CACHE BOOL
    "If ON, then don't use stack recursion when matching. See NO_RECURSE in config.h.in for details.")

SET(PCRE_POSIX_MALLOC_THRESHOLD "10" CACHE STRING
    "Threshold for malloc() usage. See POSIX_MALLOC_THRESHOLD in config.h.in for details.")

SET(PCRE_SUPPORT_UNICODE_PROPERTIES OFF CACHE BOOL
    "Enable support for Unicode properties. (If set, UTF-8 support will be enabled as well)")

SET(PCRE_SUPPORT_UTF8 OFF CACHE BOOL
    "Enable support for the Unicode UTF-8 encoding.")

SET(PCRE_SUPPORT_BSR_ANYCRLF OFF CACHE BOOL
    "ON=Backslash-R matches only LF CR and CRLF, OFF=Backslash-R matches all Unicode Linebreaks")

OPTION(PCRE_SHOW_REPORT    "Show the final configuration report" ON)
OPTION(PCRE_BUILD_PCREGREP "Build pcregrep" ON)
OPTION(PCRE_BUILD_TESTS    "Build the tests" ON)

IF (PCRE_BUILD_TESTS)
  IF (NOT PCRE_BUILD_PCREGREP)
    MESSAGE(STATUS "** Building tests requires pcregrep: PCRE_BUILD_PCREGREP forced ON")
    SET(PCRE_BUILD_PCREGREP ON)
  ENDIF(NOT PCRE_BUILD_PCREGREP)
ENDIF(PCRE_BUILD_TESTS)

IF (MINGW)
  OPTION(NON_STANDARD_LIB_PREFIX
         "ON=Shared libraries built in mingw will be named pcre.dll, etc., instead of libpcre.dll, etc."
         OFF)

  OPTION(NON_STANDARD_LIB_SUFFIX
         "ON=Shared libraries built in mingw will be named libpcre-0.dll, etc., instead of libpcre.dll, etc."
         OFF)
ENDIF(MINGW)

# bzip2 lib
IF(BZIP2_FOUND)
  OPTION (PCRE_SUPPORT_LIBBZ2 "Enable support for linking pcregrep with libbz2." ON)
ENDIF(BZIP2_FOUND)
IF(PCRE_SUPPORT_LIBBZ2)
  INCLUDE_DIRECTORIES(${BZIP2_INCLUDE_DIR})
ENDIF(PCRE_SUPPORT_LIBBZ2)

# zlib
IF(ZLIB_FOUND)
  OPTION (PCRE_SUPPORT_LIBZ "Enable support for linking pcregrep with libz." ON)
ENDIF(ZLIB_FOUND)
IF(PCRE_SUPPORT_LIBZ)
  INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
ENDIF(PCRE_SUPPORT_LIBZ)

# readline lib
IF(READLINE_FOUND)
  OPTION (PCRE_SUPPORT_LIBREADLINE  "Enable support for linking pcretest with libreadline." ON)
ENDIF(READLINE_FOUND)
IF(PCRE_SUPPORT_LIBREADLINE)
  INCLUDE_DIRECTORIES(${READLINE_INCLUDE_DIR})
ENDIF(PCRE_SUPPORT_LIBREADLINE)

# Prepare build configuration

SET(pcre_have_type_traits 0)
SET(pcre_have_bits_type_traits 0)

IF(HAVE_TYPE_TRAITS_H)
        SET(pcre_have_type_traits 1)
ENDIF(HAVE_TYPE_TRAITS_H)

IF(HAVE_BITS_TYPE_TRAITS_H)
        SET(pcre_have_bits_type_traits 1)
ENDIF(HAVE_BITS_TYPE_TRAITS_H)

SET(pcre_have_long_long 0)
SET(pcre_have_ulong_long 0)

IF(HAVE_LONG_LONG)
        SET(pcre_have_long_long 1)
ENDIF(HAVE_LONG_LONG)

IF(HAVE_UNSIGNED_LONG_LONG)
        SET(pcre_have_ulong_long 1)
ENDIF(HAVE_UNSIGNED_LONG_LONG)

IF(NOT BUILD_SHARED_LIBS)
        SET(PCRE_STATIC 1)
ENDIF(NOT BUILD_SHARED_LIBS)

IF(PCRE_SUPPORT_BSR_ANYCRLF)
        SET(BSR_ANYCRLF 1)
ENDIF(PCRE_SUPPORT_BSR_ANYCRLF)

IF(PCRE_SUPPORT_UTF8 OR PCRE_SUPPORT_UNICODE_PROPERTIES)
        SET(SUPPORT_UTF8 1)
ENDIF(PCRE_SUPPORT_UTF8 OR PCRE_SUPPORT_UNICODE_PROPERTIES)

IF(PCRE_SUPPORT_UNICODE_PROPERTIES)
        SET(SUPPORT_UCP 1)
ENDIF(PCRE_SUPPORT_UNICODE_PROPERTIES)

# This next one used to contain
#       SET(PCRETEST_LIBS ${READLINE_LIBRARY})
# but I was advised to add the NCURSES test as well, along with
# some modifications to cmake/FindReadline.cmake which should
# make it possible to override the default if necessary. PH

IF(PCRE_SUPPORT_LIBREADLINE)
        SET(SUPPORT_LIBREADLINE 1)
        SET(PCRETEST_LIBS ${READLINE_LIBRARY} ${NCURSES_LIBRARY})
ENDIF(PCRE_SUPPORT_LIBREADLINE)

IF(PCRE_SUPPORT_LIBZ)
        SET(SUPPORT_LIBZ 1)
        SET(PCREGREP_LIBS ${PCREGREP_LIBS} ${ZLIB_LIBRARIES})
ENDIF(PCRE_SUPPORT_LIBZ)

IF(PCRE_SUPPORT_LIBBZ2)
        SET(SUPPORT_LIBBZ2 1)
        SET(PCREGREP_LIBS ${PCREGREP_LIBS} ${BZIP2_LIBRARIES})
ENDIF(PCRE_SUPPORT_LIBBZ2)

SET(NEWLINE "")

IF(PCRE_NEWLINE STREQUAL "LF")
        SET(NEWLINE "10")
ENDIF(PCRE_NEWLINE STREQUAL "LF")
IF(PCRE_NEWLINE STREQUAL "CR")
        SET(NEWLINE "13")
ENDIF(PCRE_NEWLINE STREQUAL "CR")
IF(PCRE_NEWLINE STREQUAL "CRLF")
        SET(NEWLINE "3338")
ENDIF(PCRE_NEWLINE STREQUAL "CRLF")
IF(PCRE_NEWLINE STREQUAL "ANY")
        SET(NEWLINE "-1")
ENDIF(PCRE_NEWLINE STREQUAL "ANY")
IF(PCRE_NEWLINE STREQUAL "ANYCRLF")
        SET(NEWLINE "-2")
ENDIF(PCRE_NEWLINE STREQUAL "ANYCRLF")

IF(NEWLINE STREQUAL "")
        MESSAGE(FATAL_ERROR "The PCRE_NEWLINE variable must be set to one of the following values: \"LF\", \"CR\", \"CRLF\", \"ANY\", \"ANYCRLF\".")
ENDIF(NEWLINE STREQUAL "")

IF(PCRE_EBCDIC)
        SET(EBCDIC 1)
ENDIF(PCRE_EBCDIC)

IF(PCRE_NO_RECURSE)
        SET(NO_RECURSE 1)
ENDIF(PCRE_NO_RECURSE)

# Output files
CONFIGURE_FILE(config-cmake.h.in
               ${PROJECT_BINARY_DIR}/config.h
               @ONLY)

CONFIGURE_FILE(pcre.h.generic
               ${PROJECT_BINARY_DIR}/pcre.h
               COPYONLY)

# What about pcre-config and libpcre.pc?

IF(PCRE_BUILD_PCRECPP)
        CONFIGURE_FILE(pcre_stringpiece.h.in
                       ${PROJECT_BINARY_DIR}/pcre_stringpiece.h
                       @ONLY)

        CONFIGURE_FILE(pcrecpparg.h.in
                       ${PROJECT_BINARY_DIR}/pcrecpparg.h
                       @ONLY)
ENDIF(PCRE_BUILD_PCRECPP)

# Character table generation

OPTION(PCRE_REBUILD_CHARTABLES "Rebuild char tables" OFF)
IF(PCRE_REBUILD_CHARTABLES)
  ADD_EXECUTABLE(dftables dftables.c)

  GET_TARGET_PROPERTY(DFTABLES_EXE dftables LOCATION)

  ADD_CUSTOM_COMMAND(
    COMMENT "Generating character tables (pcre_chartables.c) for current locale"
    DEPENDS dftables
    COMMAND ${DFTABLES_EXE}
    ARGS        ${PROJECT_BINARY_DIR}/pcre_chartables.c
    OUTPUT      ${PROJECT_BINARY_DIR}/pcre_chartables.c
  )
ELSE(PCRE_REBUILD_CHARTABLES)
  CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/pcre_chartables.c.dist
                    ${PROJECT_BINARY_DIR}/pcre_chartables.c
                    COPYONLY)
ENDIF(PCRE_REBUILD_CHARTABLES)

# Source code

SET(PCRE_HEADERS ${PROJECT_BINARY_DIR}/pcre.h)

SET(PCRE_SOURCES
  ${PROJECT_BINARY_DIR}/pcre_chartables.c
  pcre_compile.c
  pcre_config.c
  pcre_dfa_exec.c
  pcre_exec.c
  pcre_fullinfo.c
  pcre_get.c
  pcre_globals.c
  pcre_info.c
  pcre_newline.c
  pcre_maketables.c
  pcre_ord2utf8.c
  pcre_refcount.c
  pcre_study.c
  pcre_tables.c
  pcre_try_flipped.c
  pcre_ucd.c
  pcre_valid_utf8.c
  pcre_version.c
  pcre_xclass.c
)

SET(PCREPOSIX_HEADERS pcreposix.h)

SET(PCREPOSIX_SOURCES pcreposix.c)

SET(PCRECPP_HEADERS
  pcrecpp.h
  pcre_scanner.h
  ${PROJECT_BINARY_DIR}/pcrecpparg.h
  ${PROJECT_BINARY_DIR}/pcre_stringpiece.h
)

SET(PCRECPP_SOURCES
        pcrecpp.cc
        pcre_scanner.cc
        pcre_stringpiece.cc
)

# Build setup

ADD_DEFINITIONS(-DHAVE_CONFIG_H)

IF(MSVC)
        ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
ENDIF(MSVC)

SET(CMAKE_INCLUDE_CURRENT_DIR 1)
# needed to make sure to not link debug libs
# against release libs and vice versa
IF(WIN32)
  SET(CMAKE_DEBUG_POSTFIX "d")
ENDIF(WIN32)

SET(targets)

# Libraries
# pcre
ADD_LIBRARY(pcre ${PCRE_HEADERS} ${PCRE_SOURCES} ${PROJECT_BINARY_DIR}/config.h)
SET(targets ${targets} pcre)
ADD_LIBRARY(pcreposix ${PCREPOSIX_HEADERS} ${PCREPOSIX_SOURCES})
SET(targets ${targets} pcreposix)
TARGET_LINK_LIBRARIES(pcreposix pcre)
IF(MINGW AND NOT PCRE_STATIC)
  IF(NON_STANDARD_LIB_PREFIX)
    SET_TARGET_PROPERTIES(pcre pcreposix
                        PROPERTIES PREFIX ""
    )
  ENDIF(NON_STANDARD_LIB_PREFIX)

  IF(NON_STANDARD_LIB_SUFFIX)
    SET_TARGET_PROPERTIES(pcre pcreposix
                        PROPERTIES SUFFIX "-0.dll"
    )
  ENDIF(NON_STANDARD_LIB_SUFFIX)
ENDIF(MINGW AND NOT PCRE_STATIC)


# pcrecpp
IF(PCRE_BUILD_PCRECPP)
  ADD_LIBRARY(pcrecpp ${PCRECPP_HEADERS} ${PCRECPP_SOURCES})
SET(targets ${targets} pcrecpp)
  TARGET_LINK_LIBRARIES(pcrecpp pcre)

  IF(MINGW AND NOT PCRE_STATIC)
    IF(NON_STANDARD_LIB_PREFIX)
      SET_TARGET_PROPERTIES(pcrecpp
                            PROPERTIES PREFIX ""
      )
    ENDIF(NON_STANDARD_LIB_PREFIX)

    IF(NON_STANDARD_LIB_SUFFIX)
      SET_TARGET_PROPERTIES(pcrecpp
                          PROPERTIES SUFFIX "-0.dll"
      )
    ENDIF(NON_STANDARD_LIB_SUFFIX)
  ENDIF(MINGW AND NOT PCRE_STATIC)
ENDIF(PCRE_BUILD_PCRECPP)


# Executables

# Removed by PH (2008-01-23) because pcredemo shouldn't really be built
# automatically, and it gave trouble in some environments anyway.
# ADD_EXECUTABLE(pcredemo pcredemo.c)
# TARGET_LINK_LIBRARIES(pcredemo pcreposix)
# IF(NOT BUILD_SHARED_LIBS)
#     # make sure to not use declspec(dllimport) in static mode on windows
#         SET_TARGET_PROPERTIES(pcredemo PROPERTIES COMPILE_FLAGS "-DPCRE_STATIC")
# ENDIF(NOT BUILD_SHARED_LIBS)

IF(PCRE_BUILD_PCREGREP)
  ADD_EXECUTABLE(pcregrep pcregrep.c)
  SET(targets ${targets} pcregrep)
  TARGET_LINK_LIBRARIES(pcregrep pcreposix ${PCREGREP_LIBS})
ENDIF(PCRE_BUILD_PCREGREP)


# Testing
IF(PCRE_BUILD_TESTS)
  ENABLE_TESTING()

  ADD_EXECUTABLE(pcretest pcretest.c)
  SET(targets ${targets} pcretest)
  TARGET_LINK_LIBRARIES(pcretest pcreposix ${PCRETEST_LIBS})

  IF(PCRE_BUILD_PCRECPP)
    ADD_EXECUTABLE(pcrecpp_unittest pcrecpp_unittest.cc)
    SET(targets ${targets} pcrecpp_unittest)
    TARGET_LINK_LIBRARIES(pcrecpp_unittest pcrecpp)
    IF(MINGW AND NON_STANDARD_LIB_NAMES AND NOT PCRE_STATIC)
      SET_TARGET_PROPERTIES(pcrecpp
                        PROPERTIES PREFIX ""
      )
    ENDIF(MINGW AND NON_STANDARD_LIB_NAMES AND NOT PCRE_STATIC)


    ADD_EXECUTABLE(pcre_scanner_unittest pcre_scanner_unittest.cc)
    SET(targets ${targets} pcre_scanner_unittest)
    TARGET_LINK_LIBRARIES(pcre_scanner_unittest pcrecpp)

    ADD_EXECUTABLE(pcre_stringpiece_unittest pcre_stringpiece_unittest.cc)
    SET(targets ${targets} pcre_stringpiece_unittest)
    TARGET_LINK_LIBRARIES(pcre_stringpiece_unittest pcrecpp)
  ENDIF(PCRE_BUILD_PCRECPP)

  GET_TARGET_PROPERTY(PCREGREP_EXE pcregrep DEBUG_LOCATION)
  GET_TARGET_PROPERTY(PCRETEST_EXE pcretest DEBUG_LOCATION)

  # Write out a CTest configuration file that sets some needed environment
  # variables for the test scripts.
  #
  FILE(WRITE ${PROJECT_BINARY_DIR}/CTestCustom.ctest
  "# This is a generated file.
  SET(ENV{srcdir} ${PROJECT_SOURCE_DIR})
  SET(ENV{pcregrep} ${PCREGREP_EXE})
  SET(ENV{pcretest} ${PCRETEST_EXE})
  ")

  IF(UNIX)
          ADD_TEST(pcre_test      ${PROJECT_SOURCE_DIR}/RunTest)
          ADD_TEST(pcre_grep_test ${PROJECT_SOURCE_DIR}/RunGrepTest)
  ENDIF(UNIX)
  IF(WIN32)
          ADD_TEST(pcre_test cmd /C ${PROJECT_SOURCE_DIR}/RunTest.bat)
  ENDIF(WIN32)

  GET_TARGET_PROPERTY(PCRECPP_UNITTEST_EXE
                      pcrecpp_unittest
                      DEBUG_LOCATION)

  GET_TARGET_PROPERTY(PCRE_SCANNER_UNITTEST_EXE
                      pcre_scanner_unittest
                      DEBUG_LOCATION)

  GET_TARGET_PROPERTY(PCRE_STRINGPIECE_UNITTEST_EXE
                      pcre_stringpiece_unittest
                      DEBUG_LOCATION)

  ADD_TEST(pcrecpp_test          ${PCRECPP_UNITTEST_EXE})
  ADD_TEST(pcre_scanner_test     ${PCRE_SCANNER_UNITTEST_EXE})
  ADD_TEST(pcre_stringpiece_test ${PCRE_STRINGPIECE_UNITTEST_EXE})
ENDIF(PCRE_BUILD_TESTS)

# Installation
SET(CMAKE_INSTALL_ALWAYS 1)

INSTALL(TARGETS ${targets}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

INSTALL(FILES ${PCRE_HEADERS} ${PCREPOSIX_HEADERS} DESTINATION include)

FILE(GLOB html ${PROJECT_SOURCE_DIR}/doc/html/*.html)
FILE(GLOB man1 ${PROJECT_SOURCE_DIR}/doc/*.1)
FILE(GLOB man3 ${PROJECT_SOURCE_DIR}/doc/*.3)

IF(PCRE_BUILD_PCRECPP)
        INSTALL(FILES ${PCRECPP_HEADERS} DESTINATION include)
ELSE(PCRE_BUILD_PCRECPP)
        # Remove pcrecpp.3
        FOREACH(man ${man3})
                GET_FILENAME_COMPONENT(man_tmp ${man} NAME)
                IF(NOT man_tmp STREQUAL "pcrecpp.3")
                        SET(man3_new ${man3} ${man})
                ENDIF(NOT man_tmp STREQUAL "pcrecpp.3")
        ENDFOREACH(man ${man3})
        SET(man3 ${man3_new})
ENDIF(PCRE_BUILD_PCRECPP)


INSTALL(FILES ${man1} DESTINATION man/man1)
INSTALL(FILES ${man3} DESTINATION man/man3)
INSTALL(FILES ${html} DESTINATION share/doc/pcre/html)

# help, only for nice output
IF(BUILD_SHARED_LIBS)
  SET(BUILD_STATIC_LIBS OFF)
ELSE(BUILD_SHARED_LIBS)
  SET(BUILD_STATIC_LIBS ON)
ENDIF(BUILD_SHARED_LIBS)

IF(PCRE_SHOW_REPORT)
  STRING(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype)
  IF (CMAKE_C_FLAGS)
    SET(cfsp " ")
  ENDIF(CMAKE_C_FLAGS)
  IF (CMAKE_CXX_FLAGS)
    SET(cxxfsp " ")
  ENDIF(CMAKE_CXX_FLAGS)
  MESSAGE(STATUS "")
  MESSAGE(STATUS "")
  MESSAGE(STATUS "PCRE configuration summary:")
  MESSAGE(STATUS "")
  MESSAGE(STATUS "  Install prefix .................. : ${CMAKE_INSTALL_PREFIX}")
  MESSAGE(STATUS "  C compiler ...................... : ${CMAKE_C_COMPILER}")
  MESSAGE(STATUS "  C++ compiler .................... : ${CMAKE_CXX_COMPILER}")
  MESSAGE(STATUS "  C compiler flags ................ : ${CMAKE_C_FLAGS}${cfsp}${CMAKE_C_FLAGS_${buildtype}}")
  MESSAGE(STATUS "  C++ compiler flags .............. : ${CMAKE_CXX_FLAGS}${cxxfsp}${CMAKE_CXX_FLAGS_${buildtype}}")
  MESSAGE(STATUS "")
  MESSAGE(STATUS "  Build C++ library ............... : ${PCRE_BUILD_PCRECPP}")
  MESSAGE(STATUS "  Enable UTF-8 support ............ : ${PCRE_SUPPORT_UNICODE_PROPERTIES}")
  MESSAGE(STATUS "  Unicode properties .............. : ${PCRE_SUPPORT_UNICODE_PROPERTIES}")
  MESSAGE(STATUS "  Newline char/sequence ........... : ${PCRE_NEWLINE}")
  MESSAGE(STATUS "  \\R matches only ANYCRLF ......... : ${PCRE_SUPPORT_BSR_ANYCRLF}")
  MESSAGE(STATUS "  EBCDIC coding ................... : ${PCRE_EBCDIC}")
  MESSAGE(STATUS "  Rebuild char tables ............. : ${PCRE_REBUILD_CHARTABLES}")
  MESSAGE(STATUS "  No stack recursion .............. : ${PCRE_NO_RECURSE}")
  MESSAGE(STATUS "  POSIX mem threshold ............. : ${PCRE_POSIX_MALLOC_THRESHOLD}")
  MESSAGE(STATUS "  Internal link size .............. : ${PCRE_LINK_SIZE}")
  MESSAGE(STATUS "  Match limit ..................... : ${PCRE_MATCH_LIMIT}")
  MESSAGE(STATUS "  Match limit recursion ........... : ${PCRE_MATCH_LIMIT_RECURSION}")
  MESSAGE(STATUS "  Build shared libs ............... : ${BUILD_SHARED_LIBS}")
  MESSAGE(STATUS "  Build static libs ............... : ${BUILD_STATIC_LIBS}")
  MESSAGE(STATUS "  Build pcregrep .................. : ${PCRE_BUILD_PCREGREP}")
  MESSAGE(STATUS "  Build tests (implies pcretest) .. : ${PCRE_BUILD_TESTS}")
  IF(ZLIB_FOUND)
    MESSAGE(STATUS "  Link pcregrep with libz ......... : ${PCRE_SUPPORT_LIBZ}")
  ELSE(ZLIB_FOUND)
    MESSAGE(STATUS "  Link pcregrep with libz ......... : None" )
  ENDIF(ZLIB_FOUND)
  IF(BZIP2_FOUND)
    MESSAGE(STATUS "  Link pcregrep with libbz2 ....... : ${PCRE_SUPPORT_LIBBZ2}")
  ELSE(BZIP2_FOUND)
    MESSAGE(STATUS "  Link pcregrep with libbz2 ....... : None" )
  ENDIF(BZIP2_FOUND)
  IF(NOT PCRE_SUPPORT_LIBREADLINE)
    MESSAGE(STATUS "  Link pcretest with libreadline .. : None" )
  ELSE(NOT PCRE_SUPPORT_LIBREADLINE)
    MESSAGE(STATUS "  Link pcretest with libreadline .. : ${PCRE_SUPPORT_LIBREADLINE}")
  ENDIF(NOT PCRE_SUPPORT_LIBREADLINE)
  IF(MINGW AND NOT PCRE_STATIC)
    MESSAGE(STATUS "  Non-standard dll names (prefix) . : ${NON_STANDARD_LIB_PREFIX}")
    MESSAGE(STATUS "  Non-standard dll names (suffix) . : ${NON_STANDARD_LIB_SUFFIX}")
  ENDIF(MINGW AND NOT PCRE_STATIC)
  MESSAGE(STATUS "")
ENDIF(PCRE_SHOW_REPORT)

# end CMakeLists.txt
