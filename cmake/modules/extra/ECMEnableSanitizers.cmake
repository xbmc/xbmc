# SPDX-FileCopyrightText: 2014 Mathieu Tarral <mathieu.tarral@gmail.com>
#
# SPDX-License-Identifier: BSD-3-Clause

#[=======================================================================[.rst:
ECMEnableSanitizers
-------------------

Enable compiler sanitizer flags.

The following sanitizers are supported:

- Address Sanitizer
- Memory Sanitizer
- Thread Sanitizer
- Leak Sanitizer
- Undefined Behaviour Sanitizer
- Realtime Sanitizer

All of them are implemented in Clang, depending on your version, and
there is an work in progress in GCC, where some of them are currently
implemented.

This module will check your current compiler version to see if it
supports the sanitizers that you want to enable

Usage
=====

Simply add::

   include(ECMEnableSanitizers)

to your ``CMakeLists.txt``. Note that this module is included in
:kde-module:`KDECompilerSettings`, so projects using that module do not need to also
include this one.

The sanitizers are not enabled by default. Instead, you must set
``ECM_ENABLE_SANITIZERS`` (either in your ``CMakeLists.txt`` or on the
command line) to a semicolon-separated list of sanitizers you wish to enable.
The options are:

- address
- memory
- thread
- leak
- undefined
- fuzzer
- realtime

The sanitizers "address", "memory", "thread" and "realtime" are mutually
exclusive.  You cannot enable two of them in the same build.

"leak" requires the  "address" sanitizer.

.. note::

  To reduce the overhead induced by the instrumentation of the sanitizers, it
  is advised to enable compiler optimizations (``-O1`` or higher).

Example
=======

This is an example of usage::

  mkdir build
  cd build
  cmake -DECM_ENABLE_SANITIZERS='address;leak;undefined' ..

.. note::

  Most of the sanitizers will require Clang. To enable it, use::

    -DCMAKE_CXX_COMPILER=clang++

Since 1.3.0.
#]=======================================================================]

cmake_policy(VERSION 3.16)

# MACRO check_compiler_version
#-----------------------------
macro (check_compiler_version gcc_required_version clang_required_version msvc_required_version)
    if (
        (
            CMAKE_CXX_COMPILER_ID MATCHES "GNU"
            AND
            CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${gcc_required_version}
        )
        OR
        (
            CMAKE_CXX_COMPILER_ID MATCHES "Clang"
            AND
            CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${clang_required_version}
        )
        OR
        (
            CMAKE_CXX_COMPILER_ID MATCHES "MSVC"
            AND
            CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${msvc_required_version}
        )
    )
        # error !
        message(FATAL_ERROR "You ask to enable the sanitizer ${CUR_SANITIZER},
        but your compiler ${CMAKE_CXX_COMPILER_ID} version ${CMAKE_CXX_COMPILER_VERSION}
        does not support it !
        You should use at least GCC ${gcc_required_version}, Clang ${clang_required_version}
        or MSVC ${msvc_required_version}
        (99.99 means not implemented yet)")
    endif ()
endmacro ()

# MACRO check_compiler_support
#------------------------------
macro (enable_sanitizer_flags sanitize_option)
    if (${sanitize_option} MATCHES "address")
        check_compiler_version("4.8" "3.1" "19.28")
        if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
            set(XSAN_COMPILE_FLAGS "-fsanitize=address")
        else()
            set(XSAN_COMPILE_FLAGS "-fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
            set(XSAN_LINKER_FLAGS "asan")
        endif()
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            # fixes "note: variable tracking size limit exceeded with
            # -fvar-tracking-assignments, retrying without" (which is
            # another way of saying "I'll go ahead and compile this
            # thing _twice_") by making it unlimited
            # Reference:
            #  https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
            #  -> max-vartrack-size
            string(APPEND XSAN_COMPILE_FLAGS " --param=max-vartrack-size=0")
        endif ()
    elseif (${sanitize_option} MATCHES "thread")
        check_compiler_version("4.8" "3.1" "99.99")
        set(XSAN_COMPILE_FLAGS "-fsanitize=thread")
        set(XSAN_LINKER_FLAGS "tsan")
    elseif (${sanitize_option} MATCHES "memory")
        check_compiler_version("99.99" "3.1" "99.99")
        set(XSAN_COMPILE_FLAGS "-fsanitize=memory")
    elseif (${sanitize_option} MATCHES "leak")
        check_compiler_version("4.9" "3.4" "99.99")
        set(XSAN_COMPILE_FLAGS "-fsanitize=leak")
        set(XSAN_LINKER_FLAGS "lsan")
    elseif (${sanitize_option} MATCHES "undefined")
        check_compiler_version("4.9" "3.1" "99.99")
        set(XSAN_COMPILE_FLAGS "-fsanitize=undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls")
    elseif (${sanitize_option} MATCHES "fuzzer")
        check_compiler_version("99.99" "6.0" "99.99")
        set(XSAN_COMPILE_FLAGS "-fsanitize=fuzzer")
    elseif (${sanitize_option} MATCHES "realtime")
        check_compiler_version("99.99" "20.0" "99.99")
        set(XSAN_COMPILE_FLAGS "-fsanitize=realtime")
    else ()
        message(FATAL_ERROR "Compiler sanitizer option \"${sanitize_option}\" not supported.")
    endif ()
endmacro ()

if (ECM_ENABLE_SANITIZERS)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        # for each element of the ECM_ENABLE_SANITIZERS list
        foreach ( CUR_SANITIZER ${ECM_ENABLE_SANITIZERS} )
            # lowercase filter
            string(TOLOWER ${CUR_SANITIZER} CUR_SANITIZER)
            # check option and enable appropriate flags
            enable_sanitizer_flags ( ${CUR_SANITIZER} )
            # TODO: GCC will not link pthread library if enabled ASan
            if(CMAKE_C_COMPILER_ID MATCHES "Clang")
              set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${XSAN_COMPILE_FLAGS}" )
            endif()
            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${XSAN_COMPILE_FLAGS}" )
            if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
              link_libraries(${XSAN_LINKER_FLAGS})
            endif()
            if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
                string(REPLACE "-Wl,--no-undefined " "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ")
                string(REPLACE "-Wl,--no-undefined " "" CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ")
            endif ()
        endforeach()
    else()
        message(STATUS "Tried to enable sanitizers (-DECM_ENABLE_SANITIZERS=${ECM_ENABLE_SANITIZERS}), \
but compiler (${CMAKE_CXX_COMPILER_ID}) does not have sanitizer support")
    endif()
endif()
