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
# This module will define the following variables:
#
# PYTHON_FOUND - system has PYTHON
# PYTHON_VERSION - Python version number (Major.Minor)
# PYTHON_INCLUDE_DIRS - the python include directory
# PYTHON_LIBRARIES - The python libraries
# PYTHON_LDFLAGS - Python provided link options
#
# --------
#

# for Depends/Windows builds, set search root dir to libdir path
if(KODI_DEPENDSBUILD
   OR CMAKE_SYSTEM_NAME STREQUAL WINDOWS
   OR CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
  set(Python3_USE_STATIC_LIBS TRUE)
  set(Python3_ROOT_DIR ${libdir})

  if(KODI_DEPENDSBUILD)
    # Force set to tools/depends python version
    set(PYTHON_VER 3.11)
  endif()
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

if(KODI_DEPENDSBUILD)
  find_library(FFI_LIBRARY ffi REQUIRED)
  find_library(EXPAT_LIBRARY expat REQUIRED)
  find_library(INTL_LIBRARY intl REQUIRED)
  find_library(GMP_LIBRARY gmp REQUIRED)
  find_library(LZMA_LIBRARY lzma REQUIRED)

  if(NOT CORE_SYSTEM_NAME STREQUAL android)
    set(PYTHON_DEP_LIBRARIES pthread dl util)
    if(CORE_SYSTEM_NAME STREQUAL linux)
      # python archive built via depends requires librt for _posixshmem library
      list(APPEND PYTHON_DEP_LIBRARIES rt)
    endif()
  endif()

  list(APPEND Python3_LIBRARIES ${LZMA_LIBRARY} ${FFI_LIBRARY} ${EXPAT_LIBRARY} ${INTL_LIBRARY} ${GMP_LIBRARY} ${PYTHON_DEP_LIBRARIES})
endif()

if(Python3_FOUND)
  list(APPEND PYTHON_DEFINITIONS -DHAS_PYTHON=1)
  # These are all set for easy integration with the rest of our build system
  set(PYTHON_FOUND ${Python3_FOUND})
  set(PYTHON_INCLUDE_DIRS ${Python3_INCLUDE_DIRS})
  set(PYTHON_LIBRARIES ${Python3_LIBRARIES})
  set(PYTHON_VERSION "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}" CACHE INTERNAL "" FORCE)
  set(PYTHON_LDFLAGS ${Python3_LINK_OPTIONS})
endif()

mark_as_advanced(PYTHON_EXECUTABLE PYTHON_VERSION PYTHON_INCLUDE_DIRS PYTHON_LDFLAGS LZMA_LIBRARY FFI_LIBRARY EXPAT_LIBRARY INTL_LIBRARY GMP_LIBRARY)
