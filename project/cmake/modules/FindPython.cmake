# - Try to find python
# Once done this will define
#
# PYTHON_FOUND - system has PYTHON
# PYTHON_INCLUDE_DIRS - the python include directory
# PYTHON_LIBRARIES - The python libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_PYTHON python >= 2.7 QUIET)
endif()

find_program(PYTHON_EXECUTABLE python ONLY_CMAKE_FIND_ROOT_PATH)
find_library(PYTHON_LIBRARY NAMES python2.7 PATHS ${PC_PYTHON_LIBDIR})
find_path(PYTHON_INCLUDE_DIR NAMES Python.h PATHS ${PC_PYTHON_INCLUDE_DIRS} ${DEPENDS_PATH}/include/python2.7)

if(CMAKE_CROSSCOMPILING)
  find_library(FFI_LIBRARY ffi REQUIRED)
  find_library(EXPAT_LIBRARY expat REQUIRED)
  find_library(INTL_LIBRARY intl REQUIRED)
  find_library(GMP_LIBRARY gmp REQUIRED)

  if(NOT CORE_SYSTEM_NAME STREQUAL android)
    set(PYTHON_DEP_LIBRARIES -lpthread -ldl -lutil)
  endif()

  set(PYTHON_LIBRARIES ${PYTHON_LIBRARY} ${FFI_LIBRARY} ${EXPAT_LIBRARY} ${INTL_LIBRARY} ${GMP_LIBRARY} ${PYTHON_DEP_LIBRARIES})
else()
  find_package(PythonLibs 2.7 REQUIRED)
endif()


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PYTHON REQUIRED_VARS PYTHON_INCLUDE_DIR PYTHON_LIBRARY PYTHON_LIBRARIES)
if(PYTHON_FOUND)
  set(PYTHON_INCLUDE_DIRS ${PYTHON_INCLUDE_DIR})
endif()

mark_as_advanced(PYTHON_EXECUTABLE PYTHON_INCLUDE_DIRS PYTHON_INCLUDE_DIR PYTHON_LIBRARY PYTHON_LIBRARIES PYTHON_LDFLAGS FFI_LIBRARY EXPAT_LIBRARY INTL_LIBRARY GMP_LIBRARY)
