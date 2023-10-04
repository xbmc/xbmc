# FindPython
# --------
# Finds Python3 Interpreter
#
# This module will search for a Python3 Interpreter
#
# --------
#
# the following variables influence behaviour:
#
# PYTHON_INTERPRETER_PATH - use external python not found in system paths
#                           usage: -DPYTHON_INTERPRETER_PATH=/path/to/python3
#
# --------
#
# This will define the following variable:
#
#  PYTHON_EXECUTABLE - The HOST python executable
#

# We limit search paths to rule out TARGET paths that will populate the default cmake/package paths
# Note: we do not do a find_package call as it will populate targets based on HOST
# information and pollute the TARGET python searches when required for actual target platform.
find_program(PYTHON3_INTERPRETER_EXECUTABLE NAMES python3 python
                                            HINTS ${PYTHON_INTERPRETER_PATH} ${NATIVEPREFIX}/bin
                                            NO_CACHE
                                            NO_PACKAGE_ROOT_PATH
                                            NO_CMAKE_PATH
                                            NO_CMAKE_ENVIRONMENT_PATH
                                            NO_CMAKE_INSTALL_PREFIX)

if(PYTHON3_INTERPRETER_EXECUTABLE)
  execute_process(COMMAND "${PYTHON3_INTERPRETER_EXECUTABLE}" --version
                  OUTPUT_VARIABLE PYTHON3_INTERPRETER_VERSION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX REPLACE "^Python (.*)" "\\1" PYTHON3_INTERPRETER_VERSION "${PYTHON3_INTERPRETER_VERSION}")

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PythonInterpreter
                                    REQUIRED_VARS PYTHON3_INTERPRETER_EXECUTABLE PYTHON3_INTERPRETER_VERSION
                                    VERSION_VAR PYTHON3_INTERPRETER_VERSION)

  if(PythonInterpreter_FOUND)
    # We explicitly use a CACHE variable instead of a TARGET as execute_command is not
    # able to use a TARGET - https://gitlab.kitware.com/cmake/cmake/-/issues/18364
    set(PYTHON_EXECUTABLE ${PYTHON3_INTERPRETER_EXECUTABLE} CACHE FILEPATH "Host Python interpreter" FORCE)
  else()
    if(PythonInterpreter_FIND_REQUIRED)
      message(FATAL_ERROR "A python3 interpreter was not found. Consider providing path using -DPYTHON_INTERPRETER_PATH=<path/to/python3>")
    endif()
  endif()
else()
  if(PythonInterpreter_FIND_REQUIRED)
    message(FATAL_ERROR "A python3 interpreter was not found. Consider providing path using -DPYTHON_INTERPRETER_PATH=<path/to/python3>")
  endif()
endif()
