#.rst:
# FindUdfread
# --------
# Finds the udfread library
#
# This will define the following target:
#
#   udfread::udfread - The libudfread library

if(NOT TARGET udfread::udfread)
  if(WIN32 OR WINDOWS_STORE)
    include(FindPackageMessage)

    find_package(libudfread CONFIG)

    if(libudfread_FOUND)
      # Specifically tailored to kodi windows cmake config - Debug and RelWithDebInfo available
      get_target_property(UDFREAD_LIBRARY_RELEASE libudfread::libudfread IMPORTED_LOCATION_RELWITHDEBINFO)
      get_target_property(UDFREAD_LIBRARY_DEBUG libudfread::libudfread IMPORTED_LOCATION_DEBUG)
      get_target_property(UDFREAD_INCLUDE_DIR libudfread::libudfread INTERFACE_INCLUDE_DIRECTORIES)

      include(SelectLibraryConfigurations)
      select_library_configurations(UDFREAD)

      find_package_handle_standard_args(Udfread
                                        REQUIRED_VARS UDFREAD_LIBRARY UDFREAD_INCLUDE_DIR
                                        VERSION_VAR UDFREAD_VERSION)
    endif()
  else()
    find_package(PkgConfig)
    pkg_check_modules(udfread libudfread IMPORTED_TARGET GLOBAL QUIET)

    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC udfread)
    SETUP_BUILD_VARS()

    # Check for existing UDFREAD. If version >= UDFREAD-VERSION file version, dont build
    # A corner case, but if a linux/freebsd user WANTS to build internal udfread, build anyway
    if(NOT udfread_FOUND OR 
       (udfread_VERSION VERSION_LESS ${${MODULE}_VER} AND ENABLE_INTERNAL_UDFREAD) OR
       ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_UDFREAD))

      set(UDFREAD_VERSION ${${MODULE}_VER})
      set(BUILD_NAME udfread_build)

      set(CONFIGURE_COMMAND autoreconf -vif &&
                            ./configure
                            --enable-static
                            --disable-shared
                            --prefix=${DEPENDS_PATH})
      set(BUILD_IN_SOURCE 1)

      BUILD_DEP_TARGET()
    elseif(udfread_FOUND)
      get_target_property(UDFREAD_LIBRARY PkgConfig::udfread INTERFACE_LINK_LIBRARIES)
      get_target_property(UDFREAD_INCLUDE_DIR PkgConfig::udfread INTERFACE_INCLUDE_DIRECTORIES)
      set(UDFREAD_VERSION ${udfread_VERSION})
    endif()

    if(udfread_FOUND OR TARGET udfread_build)
      set(UDFREAD_FOUND ${udfread_FOUND})
      include(FindPackageMessage)
      find_package_message(udfread "Found udfread: ${UDFREAD_LIBRARY} (version: \"${UDFREAD_VERSION}\")" "[${UDFREAD_LIBRARY}][${UDFREAD_INCLUDE_DIR}]")
    endif()

  endif()

  # pkgconfig populate target that is sufficient version
  if(TARGET PkgConfig::udfread AND NOT TARGET udfread_build)
    add_library(udfread::udfread ALIAS PkgConfig::udfread)
    set_target_properties(PkgConfig::udfread PROPERTIES
                                             INTERFACE_COMPILE_DEFINITIONS HAS_UDFREAD=1)
  # windows cmake config populated target
  elseif(TARGET libudfread::libudfread)
    add_library(udfread::udfread ALIAS libudfread::libudfread)
    set_target_properties(libudfread::libudfread PROPERTIES
                                                 INTERFACE_COMPILE_DEFINITIONS HAS_UDFREAD=1)
  # otherwise we are building
  elseif(TARGET udfread_build)
    add_library(udfread::udfread UNKNOWN IMPORTED)
    set_target_properties(udfread::udfread PROPERTIES
                                           IMPORTED_LOCATION "${UDFREAD_LIBRARY}"
                                           INTERFACE_INCLUDE_DIRECTORIES "${UDFREAD_INCLUDE_DIR}"
                                           INTERFACE_COMPILE_DEFINITIONS HAS_UDFREAD=1)
    add_dependencies(udfread::udfread udfread_build)
  endif()

  if(TARGET udfread::udfread)
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP udfread::udfread)
  endif()
endif()
