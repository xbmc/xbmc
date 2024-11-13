# FindPython
# --------
# Finds Python3 libraries
#
# This module will search for the required python libraries on the system
# If multiple versions are found, the highest version will be used.
#
# --------
#
# the following variables influence behaviour:
#
# PYTHON_PATH - use external python not found in system paths
#               usage: -DPYTHON_PATH=/path/to/python/lib
# PYTHON_VER - use exact python version, fail if not found
#               usage: -DPYTHON_VER=3.8
#
# --------
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::Python - The Python library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  # for Depends/Windows builds, set search root dir to libdir path
  if(KODI_DEPENDSBUILD
     OR CMAKE_SYSTEM_NAME STREQUAL WINDOWS
     OR CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
    set(Python3_USE_STATIC_LIBS TRUE)
    set(Python3_ROOT_DIR ${libdir})
  endif()

  # Provide root dir to search for Python if provided
  if(PYTHON_PATH)
    set(Python3_ROOT_DIR ${PYTHON_PATH})

    # unset cache var so we can generate again with a different dir (or none) if desired
    unset(PYTHON_PATH CACHE)
  endif()

  # Set specific version of Python to find if provided
  if(PYTHON_VER)
    set(VERSION ${PYTHON_VER})
    set(EXACT_VER "EXACT")

    # unset cache var so we can generate again with a different ver (or none) if desired
    unset(PYTHON_VER CACHE)
  endif()

  find_package(Python3 ${VERSION} ${EXACT_VER} COMPONENTS Development)

  if(Python3_FOUND)
    if(KODI_DEPENDSBUILD)
      find_library(EXPAT_LIBRARY expat REQUIRED)
      find_library(FFI_LIBRARY ffi REQUIRED)
      find_library(GMP_LIBRARY gmp REQUIRED)
      find_library(INTL_LIBRARY intl REQUIRED)
      find_library(LZMA_LIBRARY lzma REQUIRED)

      if(NOT CORE_SYSTEM_NAME STREQUAL android)
        set(PYTHON_DEP_LIBRARIES pthread dl util)
        if(CORE_SYSTEM_NAME STREQUAL linux)
          # python archive built via depends requires librt for _posixshmem library
          list(APPEND PYTHON_DEP_LIBRARIES rt)
        endif()
      endif()

      set(Py_LINK_LIBRARIES ${EXPAT_LIBRARY} ${FFI_LIBRARY} ${GMP_LIBRARY} ${INTL_LIBRARY} ${LZMA_LIBRARY} ${PYTHON_DEP_LIBRARIES})
    endif()

    # We use this all over the place. Maybe it would be nice to keep it as a TARGET property
    # but for now a cached variable will do
    set(PYTHON_VERSION "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}" CACHE INTERNAL "" FORCE)

    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${Python3_LIBRARIES}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${Python3_INCLUDE_DIRS}"
                                                                     INTERFACE_LINK_OPTIONS "${Python3_LINK_OPTIONS}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAS_PYTHON)

    if(Py_LINK_LIBRARIES)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_LINK_LIBRARIES "${Py_LINK_LIBRARIES}")
    endif()
  endif()
endif()
