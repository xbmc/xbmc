#.rst:
# FindNFS
# -------
# Finds the libnfs library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::NFS   - The libnfs library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  macro(buildmacroNFS)
    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                   -DENABLE_TESTS=OFF
                   -DENABLE_DOCUMENTATION=OFF
                   -DENABLE_UTILS=OFF
                   -DENABLE_EXAMPLES=OFF
                   -DCMAKE_POLICY_VERSION_MINIMUM=3.5)

    if(WIN32 OR WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_C_FLAGS "/sdl-")
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CXX_FLAGS "/sdl-")

      set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE}/01-MSUWP-compat.patch")
      generate_patchcommand("${patches}")
    endif()

    BUILD_DEP_TARGET()

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAS_NFS_MOUNT_GETEXPORTS_TIMEOUT)
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libnfs)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  # Check for existing LIBNFS. If version >= LIBNFS-VERSION file version, dont build
  # A corner case, but if a linux/freebsd user WANTS to build internal libnfs, build anyway
  if((${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_NFS) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_NFS))
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  else()
    if(TARGET libnfs::nfs)
      # This is for the case where a distro provides a non standard (Debug/Release) config type
      # convert this back to either DEBUG/RELEASE or just RELEASE
      # we only do this because we use find_package_handle_standard_args for config time output
      # and it isnt capable of handling TARGETS, so we have to extract the info
      get_target_property(_LIBNFS_CONFIGURATIONS libnfs::nfs IMPORTED_CONFIGURATIONS)
      if(_LIBNFS_CONFIGURATIONS)
        foreach(_libnfs_config IN LISTS _LIBNFS_CONFIGURATIONS)
          # Just set to RELEASE var so select_library_configurations can continue to work its magic
          string(TOUPPER ${_libnfs_config} _libnfs_config_UPPER)
          if((NOT ${_libnfs_config_UPPER} STREQUAL "RELEASE") AND
             (NOT ${_libnfs_config_UPPER} STREQUAL "DEBUG"))
            get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE libnfs::nfs IMPORTED_LOCATION_${_libnfs_config_UPPER})
          else()
            get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_${_libnfs_config_UPPER} libnfs::nfs IMPORTED_LOCATION_${_libnfs_config_UPPER})
          endif()
        endforeach()
      else()
        get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE libnfs::nfs IMPORTED_LOCATION)
      endif()

      # libnfs cmake config doesnt include INTERFACE_INCLUDE_DIRECTORIES
      find_path(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR NAMES nfsc/libnfs.h
                                                                 HINTS ${DEPENDS_PATH}/include
                                                                 ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_LINK_LIBRARIES 0 ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE)

      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} INTERFACE_INCLUDE_DIRECTORIES)

      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${libnfs_VERSION})
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(${${CMAKE_FIND_PACKAGE_NAME}_MODULE})

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(NFS
                                    REQUIRED_VARS ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(NFS_FOUND)
    # Pre existing lib, so we can run checks
    if(NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      set(CMAKE_REQUIRED_INCLUDES "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR}")
      set(CMAKE_REQUIRED_LIBRARIES ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY})

      if(CMAKE_SYSTEM_NAME MATCHES "Windows")
        set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} "ws2_32.lib")
      endif()

      # Check for mount_getexports_timeout libnfs>5.0.0
      check_cxx_source_compiles("
         ${LIBNFS_CXX_INCLUDE}
         #include <nfsc/libnfs.h>
         int main()
         {
           mount_getexports_timeout(NULL, 0);
         }
      " NFS_MOUNT_GETEXPORTS_TIMEOUT)

      if(NFS_MOUNT_GETEXPORTS_TIMEOUT)
        list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAS_NFS_MOUNT_GETEXPORTS_TIMEOUT)
      endif()

      unset(CMAKE_REQUIRED_INCLUDES)
      unset(CMAKE_REQUIRED_LIBRARIES)
    endif()

    list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAS_FILESYSTEM_NFS)

    # cmake target and not building internal
    if(TARGET libnfs::nfs AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libnfs::nfs)

      # Need to manually set this, as libnfs cmake config does not provide INTERFACE_INCLUDE_DIRECTORIES
      set_target_properties(libnfs::nfs PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR})
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      SETUP_BUILD_TARGET()

      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    ADD_TARGET_COMPILE_DEFINITION()

    ADD_MULTICONFIG_BUILDMACRO()
  endif()
endif()
