#.rst:
# FindBrotli
# ----------
# Finds the Brotli library
#
# This will define the following target ALIAS:
#
#   LIBRARY::Brotli   - The Brotli library
#

if(NOT TARGET LIBRARY::${CMAKE_FIND_PACKAGE_NAME})

  macro(buildmacroBrotli)

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/01-all-disable-exe.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/02-all-cmake-install-config.patch")

    generate_patchcommand("${patches}")
    unset(patches)

    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                   -DBROTLI_DISABLE_TESTS=ON
                   -DBROTLI_DISABLE_EXE=ON)

    BUILD_DEP_TARGET()

    # Retrieve suffix of platform byproduct to apply to second brotli library
    string(REGEX REPLACE "^.*\\." "" _LIBEXT ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT})
    if(NOT (WIN32 OR WINDOWS_STORE))
      set(_PREFIX "lib")
    endif()

    set(BROTLICOMMON_LIBRARY_RELEASE "${DEP_LOCATION}/lib/${_PREFIX}brotlicommon.${_LIBEXT}")
    set(BROTLIDEC_LIBRARY_RELEASE "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY}")

    # Todo: debug postfix libs for windows
    #       Will require patching nghttp2, as they do not use debug postfix for differentiation

  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC brotli)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  find_package(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                                                         HINTS ${DEPENDS_PATH}/share/cmake
                                                               ${DEPENDS_PATH}/lib/cmake
                                                         ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # fallback to pkgconfig to cover all bases
  if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})

    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
      pkg_check_modules(brotlicommon libbrotlicommon${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)
      if(TARGET PkgConfig::brotlicommon)
        set(brotli_VERSION ${brotlicommon_VERSION})
        pkg_check_modules(brotlidec libbrotlidec${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)

        if(TARGET PkgConfig::brotlidec)
          set(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND 1)
        endif()
      endif()
    endif()
  endif()

  # Check for existing Brotli. If version >= BROTLI-VERSION file version, dont build
  # We only build for KODI_DEPENDSBUILD or Windows platforms. Other unix builds are expected to supply system package
  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND
     (KODI_DEPENDSBUILD OR (WIN32 OR WINDOWS_STORE)) AND
     Brotli_FIND_REQUIRED)
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")

    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if((TARGET brotli::brotlicommon AND TARGET brotli::brotlidec) AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS brotli::brotlidec)
    elseif((TARGET PkgConfig::brotlicommon AND TARGET PkgConfig::brotlidec) AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::brotlidec)
    else()
      add_library(LIBRARY::brotlicommon UNKNOWN IMPORTED)
      set_target_properties(LIBRARY::brotlicommon PROPERTIES
                                                  IMPORTED_LOCATION "${BROTLICOMMON_LIBRARY_RELEASE}"
                                                  INTERFACE_INCLUDE_DIRECTORIES "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR}")
  
      add_library(LIBRARY::brotlidec UNKNOWN IMPORTED)
      set_target_properties(LIBRARY::brotlidec PROPERTIES
                                               IMPORTED_LOCATION "${BROTLIDEC_LIBRARY_RELEASE}"
                                               INTERFACE_LINK_LIBRARIES LIBRARY::brotlicommon
                                               INTERFACE_INCLUDE_DIRECTORIES "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR}")
  
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS LIBRARY::brotlidec)

      add_dependencies(LIBRARY::brotlidec ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

      # We are building as a requirement, so set LIB_BUILD property to allow calling
      # modules to know we will be building, and they will want to rebuild as well.
      # Property must be set on actual TARGET and not the ALIAS
      set_target_properties(${aliased_target} PROPERTIES LIB_BUILD ON)
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(Brotli_FIND_REQUIRED)
      message(FATAL_ERROR "Brotli libraries were not found.")
    endif()
  endif()
endif()
