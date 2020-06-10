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

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_NFS libnfs QUIET)
endif()

find_path(NFS_INCLUDE_DIR nfsc/libnfs.h
                          PATHS ${PC_NFS_INCLUDEDIR})

set(NFS_VERSION ${PC_NFS_VERSION})

find_library(NFS_LIBRARY NAMES nfs libnfs
                         PATHS ${PC_NFS_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NFS
                                  REQUIRED_VARS NFS_LIBRARY NFS_INCLUDE_DIR
                                  VERSION_VAR NFS_VERSION)

if(NFS_FOUND)
  set(NFS_LIBRARIES ${NFS_LIBRARY})
  set(NFS_INCLUDE_DIRS ${NFS_INCLUDE_DIR})
  set(NFS_DEFINITIONS -DHAS_FILESYSTEM_NFS=1)

  set(CMAKE_REQUIRED_INCLUDES "${NFS_INCLUDE_DIR}")
  set(CMAKE_REQUIRED_LIBRARIES ${NFS_LIBRARY})
  if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(NFS_CXX_INCLUDE "#include <Winsock2.h>")
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} "ws2_32.lib")
  endif()

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
    if(NFS_LIBRARY)
      set_target_properties(NFS::NFS PROPERTIES
                                     IMPORTED_LOCATION "${NFS_LIBRARY_RELEASE}")
    endif()
    set_target_properties(NFS::NFS PROPERTIES
                                   INTERFACE_INCLUDE_DIRECTORIES "${NFS_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS HAS_FILESYSTEM_NFS=1)
  endif()
endif()

mark_as_advanced(NFS_INCLUDE_DIR NFS_LIBRARY)
