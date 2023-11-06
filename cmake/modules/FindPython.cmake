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
# Python::Python3 - The python3 library
#
# --------
#

if(NOT TARGET Python::Python3)
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

    list(APPEND PYTHON_DEP_LIBRARIES ${LZMA_LIBRARY} ${FFI_LIBRARY} ${EXPAT_LIBRARY} ${INTL_LIBRARY} ${GMP_LIBRARY})
  endif()

  if(Python3_FOUND)
    set(PYTHON_VERSION "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}" CACHE INTERNAL "" FORCE)
    set(PYTHON_FOUND ${Python3_FOUND})

    add_library(Python::Python3 UNKNOWN IMPORTED)
    set_target_properties(Python::Python3 PROPERTIES
                                                 IMPORTED_LOCATION "${Python3_LIBRARIES}"
                                                 INTERFACE_INCLUDE_DIRECTORIES "${Python3_INCLUDE_DIRS}"
                                                 INTERFACE_COMPILE_DEFINITIONS HAS_PYTHON=1)

    if(PYTHON_DEP_LIBRARIES)
      set_property(TARGET Python::Python3 APPEND PROPERTY
                                                 INTERFACE_LINK_LIBRARIES "${PYTHON_DEP_LIBRARIES}")
    endif()

    if(Python3_LINK_OPTIONS)
      set_property(TARGET Python::Python3 APPEND PROPERTY
                                                 INTERFACE_LINK_LIBRARIES "${Python3_LINK_OPTIONS}")
    endif()

    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Python::Python3)

    if(KODI_DEPENDSBUILD OR WIN32)
      find_package(Pythonmodule-PIL)
      find_package(Pythonmodule-Pycryptodome)
    endif()
  endif()
endif()
