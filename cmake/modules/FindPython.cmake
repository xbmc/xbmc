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
  # for Depends builds, set search root dir to libdir path
  if(KODI_DEPENDSBUILD)
    set(Python3_USE_STATIC_LIBS TRUE)
    set(Python3_ROOT_DIR ${libdir})
  elseif(CMAKE_SYSTEM_NAME STREQUAL Windows OR CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
    # Always use the prebuilt (patched) dependency, never a host installed Python.
    # The exact version is required as FindPython3 may not know newer versions.
    set(Python3_ROOT_DIR ${DEPENDS_PATH})
    set(Python3_FIND_REGISTRY NEVER)
    file(GLOB _python_include_dir LIST_DIRECTORIES true ${DEPENDS_PATH}/include/python3.*)
    if(_python_include_dir)
      list(GET _python_include_dir 0 _python_include_dir)
      get_filename_component(_python_include_dir ${_python_include_dir} NAME)
      string(REPLACE "python" "" VERSION ${_python_include_dir})
      set(EXACT_VER "EXACT")
    endif()
    unset(_python_include_dir)
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

  find_package(Python3 ${VERSION} ${EXACT_VER} COMPONENTS Development ${SEARCH_QUIET})

  if(Python3_FOUND)
    if(KODI_DEPENDSBUILD)
      set(EXPAT_USE_STATIC_LIBS TRUE)
      find_package(EXPAT REQUIRED ${SEARCH_QUIET})

      find_library(FFI_LIBRARY ffi REQUIRED)
      find_library(GMP_LIBRARY gmp REQUIRED)

      find_package(Iconv REQUIRED ${SEARCH_QUIET})
      find_package(Intl REQUIRED ${SEARCH_QUIET})
      find_package(LibLZMA REQUIRED ${SEARCH_QUIET})

      if(NOT CORE_SYSTEM_NAME STREQUAL android)
        if(CORE_SYSTEM_NAME STREQUAL wasm)
          # Emscripten does not provide native libdl/libutil.
          # Keep only pthread for threaded Python builds.
          set(PYTHON_DEP_LIBRARIES pthread)
        else()
          set(PYTHON_DEP_LIBRARIES pthread dl util)
        endif()
        if(CORE_SYSTEM_NAME STREQUAL linux)
          # python archive built via depends requires librt for _posixshmem library
          list(APPEND PYTHON_DEP_LIBRARIES rt)
        endif()
      endif()

      set(Py_LINK_LIBRARIES EXPAT::EXPAT ${FFI_LIBRARY} ${GMP_LIBRARY} LIBRARY::Iconv Intl::Intl LibLZMA::LibLZMA ${PYTHON_DEP_LIBRARIES})
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
