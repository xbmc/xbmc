#.rst:
# FindNFS
# -------
# Finds the libnfs library
#
# This will will define the following variables::
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

include(FindPackageHandleStandardArgs)
if(NOT WIN32)
  find_library(NFS_LIBRARY NAMES nfs
                           PATHS ${PC_NFS_LIBDIR})

  find_package_handle_standard_args(NFS
                                    REQUIRED_VARS NFS_LIBRARY NFS_INCLUDE_DIR
                                    VERSION_VAR NFS_VERSION)
else()
  # Dynamically loaded DLL
  find_package_handle_standard_args(NFS
                                    REQUIRED_VARS NFS_INCLUDE_DIR
                                    VERSION_VAR NFS_VERSION)
endif()

if(NFS_FOUND)
  set(NFS_LIBRARIES ${NFS_LIBRARY})
  set(NFS_INCLUDE_DIRS ${NFS_INCLUDE_DIR})
  set(NFS_DEFINITIONS -DHAVE_LIBNFS=1)

  if(NOT TARGET NFS::NFS)
    add_library(NFS::NFS UNKNOWN IMPORTED)
    if(NFS_LIBRARY)
      set_target_properties(NFS::NFS PROPERTIES
                                     IMPORTED_LOCATION "${NFS_LIBRARY_RELEASE}")
    endif()
    set_target_properties(NFS::NFS PROPERTIES
                                   INTERFACE_INCLUDE_DIRECTORIES "${NFS_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS HAVE_LIBNFS=1)
  endif()
endif()

mark_as_advanced(NFS_INCLUDE_DIR NFS_LIBRARY)
