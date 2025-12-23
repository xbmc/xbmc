#.rst:
# FindPCRE2
# --------
# Finds the PCRE2 library
#
# This will define the following imported target::
#
#   ${APP_NAME_LC}::PCRE2    - The PCRE2 library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  macro(buildmacroPCRE2)
    set(PCRE2_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    if(WIN32)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX d)
    endif()

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/001-all-enable_docs_pc.patch")

    generate_patchcommand("${patches}")
    unset(patches)

    if(CORE_SYSTEM_NAME STREQUAL darwin_embedded OR WINDOWS_STORE)
      set(EXTRA_ARGS -DPCRE2_SUPPORT_JIT=OFF)
    else()
      set(EXTRA_ARGS -DPCRE2_SUPPORT_JIT=ON)
    endif()

    set(CMAKE_ARGS -DBUILD_STATIC_LIBS=ON
                   -DPCRE2_STATIC_PIC=ON
                   -DPCRE2_BUILD_PCRE2_8=ON
                   -DPCRE2_BUILD_PCRE2_16=OFF
                   -DPCRE2_BUILD_PCRE2_32=OFF
                   -DPCRE_NEWLINE=ANYCRLF
                   -DPCRE2_SUPPORT_UNICODE=ON
                   -DPCRE2_BUILD_PCRE2GREP=OFF
                   -DPCRE2_BUILD_TESTS=OFF
                   -DENABLE_DOCS=OFF
                   ${EXTRA_ARGS})

    set(${CMAKE_FIND_PACKAGE_NAME}_COMPILE_DEFINITIONS PCRE2_STATIC)

    BUILD_DEP_TARGET()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC pcre2)

  # PCRE2 cmake config has a hard requirement on the search name being PCRE2 even though
  # the configs are lowercase pcre2
  set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME PCRE2)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  if(KODI_DEPENDSBUILD OR (WIN32 OR WINDOWS_STORE))
    set(PCRE2_USE_STATIC_LIBS ON)
  endif()

  # Check for existing PCRE2. We specifically request a COMPONENT
  find_package(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                                                         COMPONENTS 8BIT)

  # fallback to pkgconfig for non windows platforms
  if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})

    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
      pkg_check_modules(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} libpcre2-8${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)
    endif()
  endif()

  if(("${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_PCRE2) OR
     (((CORE_SYSTEM_NAME STREQUAL linux AND NOT "webos" IN_LIST CORE_PLATFORM_NAME_LC) OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_PCRE2))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET PCRE2::8BIT AND NOT TARGET ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      get_target_property(_pcre2_ALIASTARGET PCRE2::8BIT ALIASED_TARGET)
      if(_pcre2_ALIASTARGET)
        set(_pcre2_target_name ${_pcre2_ALIASTARGET})
      else()
        set(_pcre2_target_name PCRE2::8BIT)
      endif()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${_pcre2_target_name})
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      SETUP_BUILD_TARGET()
      ADD_TARGET_COMPILE_DEFINITION()
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(PCRE2_FIND_REQUIRED)
      message(FATAL_ERROR "PCRE2 not found. Possibly use -DENABLE_INTERNAL_PCRE2=ON to build PCRE2")
    endif()
  endif()
endif()
