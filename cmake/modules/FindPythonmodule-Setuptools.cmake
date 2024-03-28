# FindPythonModule-Setuptools
# --------
# Finds/Builds Setuptools Python package
#
# This module will build the python module on the system
#
# --------
#
# This module will define the following variables:
#
# Python::Setuptools - The Setuptools python module
#
# --------
#

if(NOT TARGET Python::Setuptools)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  find_package(PythonInterpreter REQUIRED)

  set(MODULE_LC pythonmodule-setuptools)

  SETUP_BUILD_VARS()

  set(BYPASS_DEP_BUILDENV ON)

  # Set Target Configure command
  # Must be "" if no step required otherwise will try and use cmake command
  set(CONFIGURE_COMMAND COMMAND "")

  # Set Target Build command
  # Must be "" if no step required otherwise will try and use cmake command
  set(BUILD_COMMAND COMMAND "")

  # Set Target Install command
  set(INSTALL_COMMAND ${CMAKE_COMMAND} -E env ${PROJECT_TARGETENV} ${PROJECT_BUILDENV}
                      ${PYTHON_EXECUTABLE} setup.py install --prefix=${DEPENDS_PATH})

  set(BUILD_IN_SOURCE 1)
  BUILD_DEP_TARGET()

  add_library(Python::Setuptools UNKNOWN IMPORTED)
  if(TARGET ${MODULE_LC})
    add_dependencies(Python::Setuptools ${MODULE_LC})
    add_dependencies(${MODULE_LC} Python::Python3)
  endif()

  message(STATUS "Building python module Setuptools internally")

  if(NOT TARGET python-binarymodules)
    add_custom_target(python-binarymodules)
    set_target_properties(python-binarymodules PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()

  add_dependencies(python-binarymodules Python::Setuptools)
endif()