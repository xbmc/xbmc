#.rst:
# FindPCRE
# --------
# Finds the PCRECPP library
#
# This will define the following variables::
#
# PCRE_FOUND - system has libpcrecpp
# PCRE_INCLUDE_DIRS - the libpcrecpp include directory
# PCRE_LIBRARIES - the libpcrecpp libraries
# PCRE_DEFINITIONS - the libpcrecpp definitions
#
# and the following imported targets::
#
#   PCRE::PCRECPP - The PCRECPP library
#   PCRE::PCRE    - The PCRE library

if(NOT PCRE::PCRE)
  if(ENABLE_INTERNAL_PCRE)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC pcre)

    SETUP_BUILD_VARS()

    # Check for existing PCRE. If version >= PCRE-VERSION file version, dont build
    find_package(PCRE CONFIG QUIET)

    if(PCRE_VERSION VERSION_LESS ${${MODULE}_VER})

      set(PCRE_VERSION ${${MODULE}_VER})
      set(PCRE_DEBUG_POSTFIX d)

      set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/001-all-cmakeconfig.patch"
                  "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/002-all-enable_docs_pc.patch"
                  "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/003-all-postfix.patch"
                  "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/004-win-pdb.patch"
                  "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/jit_aarch64.patch")

      if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
        list(APPEND patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/tvos-bitcode-fix.patch"
                            "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/ios-clear_cache.patch")
      endif()

      generate_patchcommand("${patches}")

      set(CMAKE_ARGS -DPCRE_NEWLINE=ANYCRLF
                     -DPCRE_NO_RECURSE=ON
                     -DPCRE_MATCH_LIMIT_RECURSION=1500
                     -DPCRE_SUPPORT_JIT=ON
                     -DPCRE_SUPPORT_PCREGREP_JIT=ON
                     -DPCRE_SUPPORT_UTF=ON
                     -DPCRE_SUPPORT_UNICODE_PROPERTIES=ON
                     -DPCRE_SUPPORT_LIBZ=OFF
                     -DPCRE_SUPPORT_LIBBZ2=OFF
                     -DPCRE_BUILD_PCREGREP=OFF
                     -DPCRE_BUILD_TESTS=OFF)

      if(WIN32 OR WINDOWS_STORE)
        list(APPEND CMAKE_ARGS -DINSTALL_MSVC_PDB=ON)
      elseif(CORE_SYSTEM_NAME STREQUAL android)
        # CMake CheckFunctionExists incorrectly detects strtoq for android
        list(APPEND CMAKE_ARGS -DHAVE_STRTOQ=0)
      endif()

      # populate PCRECPP lib without a separate module
      if(NOT CORE_SYSTEM_NAME MATCHES windows)
        # Non windows platforms have a lib prefix for the lib artifact
        set(_libprefix "lib")
      endif()
      # regex used to get platform extension (eg lib for windows, .a for unix)
      string(REGEX REPLACE "^.*\\." "" _LIBEXT ${${MODULE}_BYPRODUCT})
      set(PCRECPP_LIBRARY_DEBUG ${DEP_LOCATION}/lib/${_libprefix}pcrecpp${${MODULE}_DEBUG_POSTFIX}.${_LIBEXT})
      set(PCRECPP_LIBRARY_RELEASE ${DEP_LOCATION}/lib/${_libprefix}pcrecpp.${_LIBEXT})

      BUILD_DEP_TARGET()

    else()
      # Populate paths for find_package_handle_standard_args
      find_path(PCRE_INCLUDE_DIR pcre.h)

      find_library(PCRECPP_LIBRARY_RELEASE NAMES pcrecpp)
      find_library(PCRECPP_LIBRARY_DEBUG NAMES pcrecppd)

      find_library(PCRE_LIBRARY_RELEASE NAMES pcre)
      find_library(PCRE_LIBRARY_DEBUG NAMES pcred)
    endif()
  else()

    if(PKG_CONFIG_FOUND)
      pkg_check_modules(PC_PCRE libpcrecpp QUIET)
    endif()

    find_path(PCRE_INCLUDE_DIR pcrecpp.h
                               PATHS ${PC_PCRE_INCLUDEDIR})
    find_library(PCRECPP_LIBRARY_RELEASE NAMES pcrecpp
                                         PATHS ${PC_PCRE_LIBDIR})
    find_library(PCRE_LIBRARY_RELEASE NAMES pcre
                                      PATHS ${PC_PCRE_LIBDIR})
    find_library(PCRECPP_LIBRARY_DEBUG NAMES pcrecppd
                                       PATHS ${PC_PCRE_LIBDIR})
    find_library(PCRE_LIBRARY_DEBUG NAMES pcred
                                       PATHS ${PC_PCRE_LIBDIR})
    set(PCRE_VERSION ${PC_PCRE_VERSION})

  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(PCRECPP)
  select_library_configurations(PCRE)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PCRE
                                    REQUIRED_VARS PCRECPP_LIBRARY PCRE_LIBRARY PCRE_INCLUDE_DIR
                                    VERSION_VAR PCRE_VERSION)

  if(PCRE_FOUND)
    set(PCRE_LIBRARIES ${PCRECPP_LIBRARY} ${PCRE_LIBRARY})
    set(PCRE_INCLUDE_DIRS ${PCRE_INCLUDE_DIR})
    if(WIN32)
      set(PCRE_DEFINITIONS -DPCRE_STATIC=1)
    endif()

    if(NOT TARGET PCRE::PCRE)
      add_library(PCRE::PCRE UNKNOWN IMPORTED)
      if(PCRE_LIBRARY_RELEASE)
        set_target_properties(PCRE::PCRE PROPERTIES
                                         IMPORTED_CONFIGURATIONS RELEASE
                                         IMPORTED_LOCATION "${PCRE_LIBRARY_RELEASE}")
      endif()
      if(PCRE_LIBRARY_DEBUG)
        set_target_properties(PCRE::PCRE PROPERTIES
                                         IMPORTED_CONFIGURATIONS DEBUG
                                         IMPORTED_LOCATION "${PCRE_LIBRARY_DEBUG}")
      endif()
      set_target_properties(PCRE::PCRE PROPERTIES
                                       INTERFACE_INCLUDE_DIRECTORIES "${PCRE_INCLUDE_DIR}")
      if(WIN32)
        set_target_properties(PCRE::PCRE PROPERTIES
                                         INTERFACE_COMPILE_DEFINITIONS PCRE_STATIC=1)
      endif()

    endif()
    if(NOT TARGET PCRE::PCRECPP)
      add_library(PCRE::PCRECPP UNKNOWN IMPORTED)
      if(PCRE_LIBRARY_RELEASE)
        set_target_properties(PCRE::PCRECPP PROPERTIES
                                            IMPORTED_CONFIGURATIONS RELEASE
                                            IMPORTED_LOCATION "${PCRECPP_LIBRARY_RELEASE}")
      endif()
      if(PCRE_LIBRARY_DEBUG)
        set_target_properties(PCRE::PCRECPP PROPERTIES
                                            IMPORTED_CONFIGURATIONS DEBUG
                                            IMPORTED_LOCATION "${PCRECPP_LIBRARY_DEBUG}")
      endif()
      set_target_properties(PCRE::PCRECPP PROPERTIES
                                          INTERFACE_LINK_LIBRARIES PCRE::PCRE)
    endif()
    if(TARGET pcre)
      add_dependencies(PCRE::PCRE pcre)
    endif()
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP PCRE::PCRE)
  else()
    if(PCRE_FIND_REQUIRED)
      message(FATAL_ERROR "PCRE not found. Possibly use -DENABLE_INTERNAL_PCRE=ON to build PCRE")
    endif()
  endif()

  mark_as_advanced(PCRE_INCLUDE_DIR PCRECPP_LIBRARY PCRE_LIBRARY)
endif()
