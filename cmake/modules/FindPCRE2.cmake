#.rst:
# FindPCRE2
# --------
# Finds the PCRE2 library
#
# This will define the following imported target::
#
#   ${APP_NAME_LC}::PCRE2    - The PCRE2 library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  macro(buildPCRE2)
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

    set(${CMAKE_FIND_PACKAGE_NAME}_COMPILEDEFINITIONS PCRE2_STATIC)

    BUILD_DEP_TARGET()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC pcre2)

  SETUP_BUILD_VARS()

  if(KODI_DEPENDSBUILD OR (WIN32 OR WINDOWS_STORE))
    set(PCRE2_USE_STATIC_LIBS ON)
  endif()

  # Check for existing PCRE2. If version >= PCRE2-VERSION file version, dont build
  find_package(PCRE2 CONFIG COMPONENTS 8BIT QUIET)

  if((PCRE2_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_PCRE2) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_PCRE2))
    buildPCRE2()
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
            get_target_property(PCRE2_LIBRARY_RELEASE PCRE2::8BIT IMPORTED_LOCATION_${_pcre2_config_UPPER})
          else()
            get_target_property(PCRE2_LIBRARY_${_pcre2_config_UPPER} PCRE2::8BIT IMPORTED_LOCATION_${_pcre2_config_UPPER})
          endif()
        endforeach()
      else()
        get_target_property(PCRE2_LIBRARY_RELEASE PCRE2::8BIT IMPORTED_LOCATION)
      endif()
      get_target_property(PCRE2_INCLUDE_DIR PCRE2::8BIT INTERFACE_INCLUDE_DIRECTORIES)
    else()
      # ToDo: use pkgconfig data imported and drop manual find_path/find_library
      find_package(PkgConfig QUIET)
      if(PKG_CONFIG_FOUND)
        pkg_check_modules(PC_PCRE2 libpcre2-8 QUIET)
      endif()
  
      find_path(PCRE2_INCLUDE_DIR pcre2.h
                                  HINTS ${PC_PCRE2_INCLUDEDIR})
      find_library(PCRE2_LIBRARY_RELEASE NAMES pcre2-8
                                         HINTS ${PC_PCRE2_LIBDIR})
      set(PCRE2_VERSION ${PC_PCRE2_VERSION})
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(PCRE2)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PCRE2
                                    REQUIRED_VARS PCRE2_LIBRARY PCRE2_INCLUDE_DIR
                                    VERSION_VAR PCRE2_VERSION)

  if(PCRE2_FOUND)
    if(TARGET PCRE2::8BIT AND NOT TARGET pcre2)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PCRE2::8BIT)
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      if(PCRE2_LIBRARY_RELEASE)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_CONFIGURATIONS RELEASE
                                                                         IMPORTED_LOCATION_RELEASE "${PCRE2_LIBRARY_RELEASE}")
      endif()
      if(PCRE2_LIBRARY_DEBUG)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_LOCATION_DEBUG "${PCRE2_LIBRARY_DEBUG}")
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                              IMPORTED_CONFIGURATIONS DEBUG)
      endif()
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${PCRE2_INCLUDE_DIR}")

      # Add interface compile definitions. This will usually come from an INTERNAL build being required.
      if(${CMAKE_FIND_PACKAGE_NAME}_COMPILEDEFINITIONS)
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                              INTERFACE_COMPILE_DEFINITIONS ${${CMAKE_FIND_PACKAGE_NAME}_COMPILEDEFINITIONS})
      endif()
    endif()
    if(TARGET pcre2)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} pcre2)
    endif()

    # Add internal build target when a Multi Config Generator is used
    # We cant add a dependency based off a generator expression for targeted build types,
    # https://gitlab.kitware.com/cmake/cmake/-/issues/19467
    # therefore if the find heuristics only find the library, we add the internal build
    # target to the project to allow user to manually trigger for any build type they need
    # in case only a specific build type is actually available (eg Release found, Debug Required)
    # This is mainly targeted for windows who required different runtime libs for different
    # types, and they arent compatible
    if(_multiconfig_generator)
      if(NOT TARGET pcre2)
        buildPCRE2()
        set_target_properties(pcre2 PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends pcre2)
    endif()

  else()
    if(PCRE2_FIND_REQUIRED)
      message(FATAL_ERROR "PCRE2 not found. Possibly use -DENABLE_INTERNAL_PCRE2=ON to build PCRE2")
    endif()
  endif()
endif()
