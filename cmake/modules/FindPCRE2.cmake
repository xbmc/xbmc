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

  if((${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_PCRE2) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_PCRE2))
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  else()
    # if PCRE2::8BIT target exists, it meets version requirements
    # we only do a pkgconfig search when a suitable cmake config returns nothing
    if(TARGET PCRE2::8BIT)
      get_target_property(_PCRE2_CONFIGURATIONS PCRE2::8BIT IMPORTED_CONFIGURATIONS)
      if(_PCRE2_CONFIGURATIONS)
        foreach(_pcre2_config IN LISTS _PCRE2_CONFIGURATIONS)
          # Just set to RELEASE var so select_library_configurations can continue to work its magic
          string(TOUPPER ${_pcre2_config} _pcre2_config_UPPER)
          if((NOT ${_pcre2_config_UPPER} STREQUAL "RELEASE") AND
             (NOT ${_pcre2_config_UPPER} STREQUAL "DEBUG"))
            get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE PCRE2::8BIT IMPORTED_LOCATION_${_pcre2_config_UPPER})
          else()
            get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_${_pcre2_config_UPPER} PCRE2::8BIT IMPORTED_LOCATION_${_pcre2_config_UPPER})
          endif()
        endforeach()
      else()
        get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE PCRE2::8BIT IMPORTED_LOCATION)
      endif()
      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR PCRE2::8BIT INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_LINK_LIBRARIES 0 ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE)

      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} INTERFACE_INCLUDE_DIRECTORIES)

      # Some older debian pkgconfig packages for PCRE2 dont include the include dirs data
      # If we cant get that data from the pkgconfig TARGET, fall back to the old *_INCLUDEDIR
      # variable
      if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR)
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_INCLUDEDIR)
      endif()
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(${${CMAKE_FIND_PACKAGE_NAME}_MODULE})

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PCRE2
                                    REQUIRED_VARS ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(PCRE2_FOUND)
    if(TARGET PCRE2::8BIT AND NOT TARGET ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PCRE2::8BIT)
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
