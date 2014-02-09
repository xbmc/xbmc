# This code sets the following variables:
# PYTHON_EXEC         - path to python executable
# PYTHON_LIBRARIES    - path to the python library
# PYTHON_INCLUDE_DIRS - path to where Python.h is found
# PTYHON_SITE_MODULES - path to site-packages
# PYTHON_ARCH         - name of architecture to be used for platform-specific
#                       binary modules
# PYTHON_VERSION      - version of python
#
# You can optionally include the version number when using this package
# like so:
#   find_package(python 2.6)
#
# This code defines a helper function find_python_module(). It can be used
# like so:
#   find_python_module(numpy)
# If numpy is found, the variable PY_NUMPY contains the location of the numpy
# module. If the module is required add the keyword "REQUIRED":
#   find_python_module(numpy REQUIRED)

include(FindPackageHandleStandardArgs)

# allow specifying which Python installation to use
if (NOT PYTHON_EXEC)
    set(PYTHON_EXEC $ENV{PYTHON_EXEC})
endif (NOT PYTHON_EXEC)

if (NOT PYTHON_EXEC)
    find_program(PYTHON_EXEC "python${Python_FIND_VERSION}"
        PATHS
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.1\\InstallPath]
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.0\\InstallPath]
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.7\\InstallPath]
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.6\\InstallPath]
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.5\\InstallPath]
        DOC "Location of python executable to use")
endif(NOT PYTHON_EXEC)

# if Python is still not found, return
if (NOT PYTHON_EXEC)
    # dummy function
    function(find_python_module module)
        return()
    endfunction(find_python_module)
    return()
endif()

# On OS X the python executable might be symlinked to the "real" location
# of the python executable. The header files and libraries are found relative
# to that path.
# For CMake 2.6 and below, the REALPATH option is included in the ABSOLUTE option
if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 2.6)
  get_filename_component(PYTHON_EXEC_ "${PYTHON_EXEC}" REALPATH)
else()
  get_filename_component(PYTHON_EXEC_ "${PYTHON_EXEC}" ABSOLUTE)
endif()
set(PYTHON_EXEC "${PYTHON_EXEC_}" CACHE FILEPATH "Path to Python interpreter")

string(REGEX REPLACE "/bin/python.*" "" PYTHON_PREFIX "${PYTHON_EXEC_}")
string(REGEX REPLACE "/bin/python.*" "" PYTHON_PREFIX2 "${PYTHON_EXEC}")

execute_process(COMMAND "${PYTHON_EXEC}" "-c"
    "import sys; print('%d.%d' % (sys.version_info[0],sys.version_info[1]))"
    OUTPUT_VARIABLE PYTHON_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REPLACE "." "" PYTHON_VERSION_NO_DOTS ${PYTHON_VERSION})

find_library(PYTHON_LIBRARIES 
    NAMES "python${PYTHON_VERSION_NO_DOTS}" "python${PYTHON_VERSION}"
    PATHS
        "${PYTHON_PREFIX}/lib"
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${PYTHON_VERSION}\\InstallPath]/libs
    PATH_SUFFIXES "" "python${PYTHON_VERSION}/config"
    DOC "Python libraries" NO_DEFAULT_PATH)

find_path(PYTHON_INCLUDE_DIRS "Python.h"
    PATHS
        "${PYTHON_PREFIX}/include"
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${PYTHON_VERSION}\\InstallPath]/include
    PATH_SUFFIXES python${PYTHON_VERSION} python${PYTHON_VERSION}m
    DOC "Python include directories" NO_DEFAULT_PATH)

execute_process(COMMAND "${PYTHON_EXEC}" "-c"
    "from distutils.sysconfig import get_python_lib; print(get_python_lib())"
    OUTPUT_VARIABLE PYTHON_SITE_MODULES_
    OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX REPLACE "^${PYTHON_PREFIX2}/" "" PYTHON_SITE_MODULES "${PYTHON_SITE_MODULES_}")

function(find_python_module module)
    string(TOUPPER ${module} module_upper)
    if(ARGC GREATER 1)
        if (ARGV1 STREQUAL "REQUIRED")
            set(PY_${module}_FIND_REQUIRED TRUE)
        else()
            if (ARGV1 STREQUAL "QUIET")
                set(PY_${module}_FIND_QUIETLY TRUE)
            endif()
        endif()
    endif()
    if(NOT PY_${module_upper})
        # A module's location is usually a directory, but for binary modules
        # it's a .so file.
        execute_process(COMMAND "${PYTHON_EXEC}" "-c"
            "import re, ${module}; print(re.compile('/__init__.py.*').sub('',${module}.__file__))"
            RESULT_VARIABLE _${module}_status
            OUTPUT_VARIABLE _${module}_location
            ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(NOT _${module}_status)
            set(PY_${module_upper} ${_${module}_location} CACHE STRING
                "Location of Python module ${module}")
        endif(NOT _${module}_status)
    endif(NOT PY_${module_upper})
    find_package_handle_standard_args(PY_${module} DEFAULT_MSG PY_${module_upper})
endfunction(find_python_module)

set(PYTHON_ARCH "unknown")
if(APPLE)
    set(PYTHON_ARCH "darwin")
else(APPLE)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
            set(PYTHON_ARCH "linux2")
        else(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
            set(PYTHON_ARCH "linux")
        endif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    else(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
            set(PYTHON_ARCH "windows")
        endif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
endif(APPLE)

find_package_handle_standard_args(Python DEFAULT_MSG
    PYTHON_LIBRARIES PYTHON_INCLUDE_DIRS PYTHON_SITE_MODULES)
