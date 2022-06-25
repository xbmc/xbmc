#.rst:
# FindNFS
# -------
# Finds the libnfs library
#
# This will define the following variables::
#
# NFS_FOUND - system has libnfs
# NFS_INCLUDE_DIRS - the libnfs include directory
# NFS_LIBRARIES - the libnfs libraries
# NFS_DEFINITIONS - the libnfs compile definitions
#
# and the following imported targets::
#
#   NFS::NFS   - The libnfs library

include(cmake/scripts/common/ModuleHelpers.cmake)

set(MODULE_LC libnfs)

SETUP_BUILD_VARS()

# Search for cmake config. Suitable for all platforms including windows
find_package(LIBNFS CONFIG QUIET)

if(NOT LIBNFS_FOUND)
  if(ENABLE_INTERNAL_NFS)
    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                   -DENABLE_TESTS=OFF
                   -DENABLE_DOCUMENTATION=OFF
                   -DENABLE_UTILS=OFF
                   -DENABLE_EXAMPLES=OFF)

    # Patch merged upstream. drop when a release > 5.0.1 occurs
    set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/001-fix-cmake-build.patch")

    generate_patchcommand("${patches}")

    BUILD_DEP_TARGET()

    set(NFS_LIBRARY ${${MODULE}_LIBRARY})
    set(NFS_INCLUDE_DIR ${${MODULE}_INCLUDE_DIR})
  else()
    # Try pkgconfig based search. Linux may not have a version with cmake config installed
    if(PKG_CONFIG_FOUND)
      pkg_check_modules(PC_NFS libnfs QUIET)
    endif()

    find_path(NFS_INCLUDE_DIR nfsc/libnfs.h
                              PATHS ${PC_NFS_INCLUDEDIR})

    set(LIBNFS_VERSION ${PC_NFS_VERSION})

    find_library(NFS_LIBRARY NAMES nfs libnfs
                             PATHS ${PC_NFS_LIBDIR})
  endif()
else()
  # Find lib and path as we cant easily rely on cmake-config
  find_library(NFS_LIBRARY NAMES nfs libnfs
                           PATHS ${DEPENDS_PATH}/lib)
  find_path(NFS_INCLUDE_DIR nfsc/libnfs.h PATHS ${DEPENDS_PATH}/include)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NFS
                                  REQUIRED_VARS NFS_LIBRARY NFS_INCLUDE_DIR
                                  VERSION_VAR LIBNFS_VERSION)

if(NFS_FOUND)
  set(NFS_LIBRARIES ${NFS_LIBRARY})
  set(NFS_INCLUDE_DIRS ${NFS_INCLUDE_DIR})
  set(NFS_DEFINITIONS -DHAS_FILESYSTEM_NFS=1)

  set(CMAKE_REQUIRED_INCLUDES "${NFS_INCLUDE_DIR}")
  set(CMAKE_REQUIRED_LIBRARIES ${NFS_LIBRARY})

  # Check for nfs_set_timeout
  check_cxx_source_compiles("
     ${NFS_CXX_INCLUDE}
     #include <nfsc/libnfs.h>
     int main()
     {
       nfs_set_timeout(NULL, 0);
     }
  " NFS_SET_TIMEOUT)

  if(NFS_SET_TIMEOUT)
    list(APPEND NFS_DEFINITIONS -DHAS_NFS_SET_TIMEOUT)
  endif()

  # Check for mount_getexports_timeout
  check_cxx_source_compiles("
     ${NFS_CXX_INCLUDE}
     #include <nfsc/libnfs.h>
     int main()
     {
       mount_getexports_timeout(NULL, 0);
     }
  " NFS_MOUNT_GETEXPORTS_TIMEOUT)

  if(NFS_MOUNT_GETEXPORTS_TIMEOUT)
    list(APPEND NFS_DEFINITIONS -DHAS_NFS_MOUNT_GETEXPORTS_TIMEOUT)
  endif()

  unset(CMAKE_REQUIRED_INCLUDES)
  unset(CMAKE_REQUIRED_LIBRARIES)

  if(NOT TARGET NFS::NFS)
    add_library(NFS::NFS UNKNOWN IMPORTED)

    set_target_properties(NFS::NFS PROPERTIES
                                   IMPORTED_LOCATION "${NFS_LIBRARY_RELEASE}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${NFS_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS "${NFS_DEFINITIONS}")
    if(TARGET libnfs)
      add_dependencies(NFS::NFS libnfs)
    endif()
  endif()
  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP NFS::NFS)
endif()

mark_as_advanced(NFS_INCLUDE_DIR NFS_LIBRARY)
