# This file contains functions that support the debugging of the CMake files.

# This file shouldn't be included per default in any CMake file. It should be
# included and used only on demand. All functions are prefixed with "debug_".
#
# Usage:
# include(scripts/common/CMakeHelpers.cmake)
# debug_print_variables()

# Print all CMake variables.
macro(debug_print_variables)
  get_cmake_property(_variableNames VARIABLES)
  foreach(_variableName ${_variableNames})
    message(STATUS "${_variableName} = ${${_variableName}}")
  endforeach()
endmacro()

# Get all properties that CMake supports and convert them to a list.
function(debug_get_properties VAR)
  execute_process(COMMAND cmake --help-property-list
                  OUTPUT_VARIABLE _properties)
  string(REGEX REPLACE ";" "\\\\;" _properties "${_properties}")
  string(REGEX REPLACE "\n" ";" _properties "${_properties}")
  list(REMOVE_DUPLICATES _properties)
  list(REMOVE_ITEM _properties LOCATION)
  set(${VAR} ${_properties} PARENT_SCOPE)
endfunction()

# List all properties.
function(debug_list_properties)
  debug_get_properties(_properties)
  message("CMake properties = ${_properties}")
endfunction()

# Print all set properties of a specified target.
function(debug_print_target_properties target)
  if(NOT TARGET ${target})
    message(FATAL_ERROR "There is no target named '${target}'")
  endif()

  debug_get_properties(_properties)

  # Reading LOCATION property is deprecated and triggers a fatal error.
  string(REGEX REPLACE ";LOCATION;|LOCATION" "" _properties "${_properties}")
  string(REGEX REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" _properties
         "${_properties}")
  foreach(_property ${_properties})
    get_property(_value TARGET ${target} PROPERTY ${_property} SET)
    if(_value)
      get_target_property(_value ${target} ${_property})
      message("${target} ${_property} = ${_value}")
    endif()
  endforeach()
endfunction()
