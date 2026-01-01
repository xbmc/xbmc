# FindPythonmoduleSetuptools
# --------
# Finds/Builds Setuptools Python package
#
# This module will build the python module on the system
#
# --------
#
# This module will define the following variables:
#
# Python::PythonmoduleSetuptools - The Setuptools python module
#
# --------
#

if(NOT TARGET Python::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  find_package(PythonInterpreter REQUIRED ${SEARCH_QUIET})
  # We explicitly do not search for Python as a required dependency due to recursion

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC pythonmodule-setuptools)

  SETUP_BUILD_VARS()

  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPASS_DEP_BUILDENV ON)

  message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")

  # Set Target Build command. Must be "" if no step required
  set(CONFIGURE_COMMAND COMMAND "")

  # Set Target Build command. Must be "" if no step required
  set(BUILD_COMMAND COMMAND "")

  # Set Target Install command
  set(INSTALL_COMMAND ${CMAKE_COMMAND} -E env ${PROJECT_TARGETENV} ${PROJECT_BUILDENV} PYTHONPATH=${PYTHON_SITE_PKG}
                      ${PYTHON_EXECUTABLE} setup.py install --prefix=${DEPENDS_PATH})

  set(BUILD_IN_SOURCE 1)
  BUILD_DEP_TARGET()

  if(UNIX)
    add_custom_command(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} POST_BUILD
                       COMMAND PYTHONPATH=${PYTHON_SITE_PKG} find ${PYTHON_SITE_PKG}/*setuptools* -type f -name \"*.exe\" -exec rm -f {} "\;"  || (exit 0))
  endif()

  # Set TARGET namespace to Python
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE Python)
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INTERFACE_LIB TRUE)

  SETUP_BUILD_TARGET()

  if(NOT TARGET python-modules)
    add_custom_target(python-modules)
    set_target_properties(python-modules PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()

  add_dependencies(python-modules ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE}::${CMAKE_FIND_PACKAGE_NAME})

  add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE}::${CMAKE_FIND_PACKAGE_NAME} ${APP_NAME_LC}::Python)

endif()
