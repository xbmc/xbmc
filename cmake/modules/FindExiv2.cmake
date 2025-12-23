#.rst:
# FindExiv2
# -------
# Finds the exiv2 library
#
# This will define the following imported targets::
#
#   ${APP_NAME_LC}::Exiv2   - The EXIV2 library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  macro(buildmacroExiv2)
    find_package(Brotli REQUIRED ${SEARCH_QUIET})
    find_package(Iconv REQUIRED ${SEARCH_QUIET})
    find_package(ZLIB REQUIRED ${SEARCH_QUIET})

    # Patch pending review upstream (https://github.com/Exiv2/exiv2/pull/3004)
    set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/0001-WIN-lib-postfix.patch")

    generate_patchcommand("${patches}")
    unset(patches)

    if(WIN32 OR WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX d)

      # Exiv2 cant be built using /RTC1, so we alter and disable the auto addition of flags
      # using WIN_DISABLE_PROJECT_FLAGS
      string(REPLACE "/RTC1" "" ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} )

      set(EXTRA_ARGS "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}$<$<CONFIG:Debug>: ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CXX_FLAGS_DEBUG}>$<$<CONFIG:Release>: ${CMAKE_CXX_FLAGS_RELEASE}>"
                     "-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}$<$<CONFIG:Debug>: ${CMAKE_EXE_LINKER_FLAGS_DEBUG}>$<$<CONFIG:Release>: ${CMAKE_EXE_LINKER_FLAGS_RELEASE}>")

      set(WIN_DISABLE_PROJECT_FLAGS ON)
    endif()

    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                   -DEXIV2_ENABLE_WEBREADY=OFF
                   -DEXIV2_ENABLE_XMP=OFF
                   -DEXIV2_ENABLE_CURL=OFF
                   -DEXIV2_ENABLE_NLS=OFF
                   -DEXIV2_BUILD_SAMPLES=OFF
                   -DEXIV2_BUILD_UNIT_TESTS=OFF
                   -DEXIV2_ENABLE_VIDEO=OFF
                   -DEXIV2_ENABLE_BMFF=ON
                   -DEXIV2_ENABLE_BROTLI=ON
                   -DEXIV2_ENABLE_INIH=OFF
                   -DEXIV2_ENABLE_FILESYSTEM_ACCESS=OFF
                   -DEXIV2_BUILD_EXIV2_COMMAND=OFF
                   ${EXTRA_ARGS})

    if(NOT CMAKE_CXX_COMPILER_LAUNCHER STREQUAL "")
      list(APPEND CMAKE_ARGS -DBUILD_WITH_CCACHE=ON)
    endif()

    BUILD_DEP_TARGET()

    # Link libraries for target interface
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES LIBRARY::Brotli
                                                            LIBRARY::Iconv
                                                            LIBRARY::ZLIB)

    if(CORE_SYSTEM_NAME STREQUAL "freebsd")
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES procstat)
    elseif(CORE_SYSTEM_NAME MATCHES "windows")
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES psapi)
    endif()

    # Add dependencies to build target
    add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} LIBRARY::Brotli
                                                                        LIBRARY::Iconv
                                                                        LIBRARY::ZLIB)
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  # If there is a potential this library can be built internally
  # Check its dependencies to allow forcing this lib to be built if one of its
  # dependencies requires being rebuilt
  if(ENABLE_INTERNAL_EXIV2)
    # Dependency list of this find module for an INTERNAL build
    set(${CMAKE_FIND_PACKAGE_NAME}_DEPLIST Brotli
                                           Iconv
                                           ZLIB)

    check_dependency_build(${CMAKE_FIND_PACKAGE_NAME} "${${CMAKE_FIND_PACKAGE_NAME}_DEPLIST}")
  endif()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC exiv2)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  # Check for existing EXIV2. If version >= EXIV2-VERSION file version, dont build
  if(("${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_EXIV2) OR
     (((CORE_SYSTEM_NAME STREQUAL linux AND NOT "webos" IN_LIST CORE_PLATFORM_NAME_LC) OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_EXIV2) OR
     (DEFINED ${CMAKE_FIND_PACKAGE_NAME}_FORCE_BUILD))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if((TARGET Exiv2::exiv2lib OR TARGET exiv2lib) AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      # Exiv alias exiv2lib in their latest cmake config. We test for the alias
      # to workout what we need to point OUR alias at.
      get_target_property(_EXIV2_ALIASTARGET exiv2lib ALIASED_TARGET)
      if(_EXIV2_ALIASTARGET)
        set(_exiv_target_name ${_EXIV2_ALIASTARGET})
      else()
        set(_exiv_target_name exiv2lib)
      endif()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${_exiv_target_name})
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      SETUP_BUILD_TARGET()

      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(Exiv2_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find or build Exiv2 library. You may want to try -DENABLE_INTERNAL_EXIV2=ON")
    endif()
  endif()
endif()
