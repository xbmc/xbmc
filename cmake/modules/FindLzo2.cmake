#.rst:
# FindLzo2
# --------
# Finds the Lzo2 library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Lzo2   - The Lzo2 library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroLzo2)
    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/001-all-enable_tests.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/002-all-enable_docs.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/003-all-install_pkgconfig.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/004-all-win_set_debug_postfix.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/005-all-win_set_compile_mp.patch")

    if(WIN32 OR WINDOWS_STORE)
      # Debug postfix only used for windows
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX d)
    endif()

    generate_patchcommand("${patches}")
    unset(patches)

    set(CMAKE_ARGS -DENABLE_STATIC=ON
                   -DENABLE_SHARED=OFF
                   -DCMAKE_POLICY_VERSION_MINIMUM=3.10)

    if(MSVC)
      # lzo runs ~130 configure checks one after another, each an MSBuild project under
      # Visual Studio, for minutes. These are their MSVC results.
      set(_found_headers ASSERT_H CTYPE_H ERRNO_H FCNTL_H FLOAT_H LIMITS_H MALLOC_H MEMORY_H
                         SETJMP_H SIGNAL_H STDARG_H STDDEF_H STDINT_H STDIO_H STDLIB_H STRING_H
                         SYS_STAT_H SYS_TYPES_H TIME_H)
      set(_missing_headers DIRENT_H STRINGS_H SYS_MMAN_H SYS_RESOURCE_H SYS_TIME_H SYS_WAIT_H
                           UNISTD_H UTIME_H)
      set(_found_functions ACCESS ATEXIT ATOI ATOL CHMOD CTIME DIFFTIME FSTAT GETENV GMTIME
                           ISATTY LOCALTIME LONGJMP MEMCMP MEMCPY MEMMOVE MEMSET MKDIR MKTIME QSORT
                           RAISE RMDIR SIGNAL STRCHR STRDUP STRERROR STRFTIME STRICMP STRNICMP
                           STRRCHR STRSTR TIME UMASK UTIME)
      set(_missing_functions ALLOCA CHOWN CLOCK_GETCPUCLOCKID CLOCK_GETRES CLOCK_GETTIME
                             GETPAGESIZE GETRUSAGE GETTIMEOFDAY LSTAT MMAP MPROTECT MUNMAP SNPRINTF
                             STRCASECMP STRNCASECMP VSNPRINTF)
      # an intrinsic, not a function, on arm
      if(SDK_TARGET_ARCH MATCHES "^arm")
        list(APPEND _missing_functions SETJMP)
      else()
        list(APPEND _found_functions SETJMP)
      endif()
      set(_sizes SHORT=2 INT=4 LONG=4 LONG_LONG=8 __INT16=2 __INT32=4 __INT64=8 INTMAX_T=8
                 UINTMAX_T=8 FLOAT=4 DOUBLE=8 LONG_DOUBLE=8 DEV_T=4 OFF_T=4 TIME_T=8)
      foreach(_type VOID_P SIZE_T PTRDIFF_T INTPTR_T UINTPTR_T)
        list(APPEND _sizes ${_type}=${CMAKE_SIZEOF_VOID_P})
      endforeach()

      # check_type_size checks these three itself
      set(_checks "set(HAVE_SYS_TYPES_H 1 CACHE INTERNAL \"\")\n"
                  "set(HAVE_STDINT_H 1 CACHE INTERNAL \"\")\n"
                  "set(HAVE_STDDEF_H 1 CACHE INTERNAL \"\")\n")
      foreach(_var IN LISTS _found_headers _found_functions)
        list(APPEND _checks "set(mfx_HAVE_${_var} 1 CACHE INTERNAL \"\")\n")
      endforeach()
      foreach(_var IN LISTS _missing_headers _missing_functions)
        list(APPEND _checks "set(mfx_HAVE_${_var} \"\" CACHE INTERNAL \"\")\n")
      endforeach()
      foreach(_size IN LISTS _sizes)
        string(REPLACE "=" ";" _size ${_size})
        list(GET _size 0 _type)
        list(GET _size 1 _bytes)
        list(APPEND _checks "set(HAVE_mfx_SIZEOF_${_type} TRUE CACHE INTERNAL \"\")\n"
                            "set(mfx_SIZEOF_${_type} ${_bytes} CACHE INTERNAL \"\")\n")
      endforeach()
      foreach(_type FPOS_T MODE_T SSIZE_T)
        list(APPEND _checks "set(HAVE_mfx_SIZEOF_${_type} FALSE CACHE INTERNAL \"\")\n"
                            "set(mfx_SIZEOF_${_type} \"\" CACHE INTERNAL \"\")\n")
      endforeach()

      string(CONCAT _checks ${_checks})
      file(WRITE ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/liblzo2-checks.cmake "${_checks}")
      list(APPEND CMAKE_ARGS -C${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/liblzo2-checks.cmake)
      unset(_found_headers)
      unset(_missing_headers)
      unset(_found_functions)
      unset(_missing_functions)
      unset(_sizes)
      unset(_checks)
    endif()

    BUILD_DEP_TARGET()
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC liblzo2)
  set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME lzo2)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(("${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_LZO2) OR
     (((CORE_SYSTEM_NAME STREQUAL linux AND NOT "webos" IN_LIST CORE_PLATFORM_NAME_LC) OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_LZO2))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    elseif(TARGET lzo2::lzo2 AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      # Kodi target - windows prebuilt lib
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS lzo2::lzo2)
    else()
      SETUP_BUILD_TARGET()

      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(Lzo2_FIND_REQUIRED)
      message(FATAL_ERROR "Lzo2 library was not found.")
    endif()
  endif()
endif()
