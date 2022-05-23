#.rst:
# FindLzo2
# --------
# Finds the Lzo2 library
#
# This will define the following variables::
#
# LZO2_FOUND - system has Lzo2
# LZO2_INCLUDE_DIRS - the Lzo2 include directory
# LZO2_LIBRARIES - the Lzo2 libraries
#
# and the following imported targets::
#
#   Lzo2::Lzo2   - The Lzo2 library

if(NOT Lzo2::Lzo2)
  if(ENABLE_INTERNAL_LZO2)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC lzo2)

    SETUP_BUILD_VARS()

    # Check for existing LZO2. If version >= LZO2-VERSION file version, dont build
    find_package(LZO2 CONFIG QUIET)

    if(LZO2_VERSION VERSION_LESS ${${MODULE}_VER})

      set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/001-all-enable_tests.patch"
                  "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/002-all-enable_docs.patch"
                  "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/003-all-cmakeconfig.patch")

      if(WIN32 OR WINDOWS_STORE)
        list(APPEND patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/004-win-pdb.patch")

        # Debug postfix only used for windows
        set(LZO2_DEBUG_POSTFIX d)
      endif()

      generate_patchcommand("${patches}")

      set(CMAKE_ARGS -DENABLE_STATIC=ON
                     -DENABLE_SHARED=OFF)

      BUILD_DEP_TARGET()
    else()
      # Populate paths for find_package_handle_standard_args
      find_path(LZO2_INCLUDE_DIR NAMES lzo/lzo1x.h)
      find_library(LZO2_LIBRARY_RELEASE NAMES lzo2)
    endif()
  else()
    find_path(LZO2_INCLUDE_DIR NAMES lzo1x.h
                               PATH_SUFFIXES lzo)

    find_library(LZO2_LIBRARY_RELEASE NAMES lzo2 liblzo2)
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(LZO2)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Lzo2
                                    REQUIRED_VARS LZO2_LIBRARY LZO2_INCLUDE_DIR
                                    VERSION_VAR LZO2_VER)

  if(LZO2_FOUND)
    set(LZO2_LIBRARIES ${LZO2_LIBRARY})
    set(LZO2_INCLUDE_DIRS ${LZO2_INCLUDE_DIR})

    if(NOT TARGET Lzo2::Lzo2)
      add_library(Lzo2::Lzo2 UNKNOWN IMPORTED)
      if(LZO2_LIBRARY_RELEASE)
        set_target_properties(Lzo2::Lzo2 PROPERTIES
                                         IMPORTED_CONFIGURATIONS RELEASE
                                         IMPORTED_LOCATION "${LZO2_LIBRARY_RELEASE}")
      endif()
      if(LZO2_LIBRARY_DEBUG)
        set_target_properties(Lzo2::Lzo2 PROPERTIES
                                         IMPORTED_CONFIGURATIONS DEBUG
                                         IMPORTED_LOCATION "${LZO2_LIBRARY_DEBUG}")
      endif()
      set_target_properties(Lzo2::Lzo2 PROPERTIES
                                       INTERFACE_INCLUDE_DIRECTORIES "${LZO2_INCLUDE_DIR}")
    endif()
    if(TARGET lzo2)
      add_dependencies(Lzo2::Lzo2 lzo2)
    endif()
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Lzo2::Lzo2)
  endif()

  mark_as_advanced(LZO2_INCLUDE_DIR LZO2_LIBRARY)
endif()
