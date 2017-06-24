# Marks header file as being overridden on a certain list of platforms.
#
# Explicitly marking a file as overridden on specific platforms avoids issues with globbing where
# CMake would have to be called manually when overriding for a new platform.
#
# Usage: add_platform_override(${PROJECT_NAME} settings.h PLATFORMS android X11 osx)
function(add_platform_override target filename)
  cmake_parse_arguments(ARG "" "" "PLATFORMS" ${ARGN})
  if(NOT ARG_PLATFORMS)
    message(FATAL_ERROR "Missing parameter PLATFORMS")
  endif()

  # Generate an _override.h header that is either empty (platform doesn't define overrides)
  # or includes the corresponding platform override header.
  # This _override.h has to be included by the generic header.

  # Determine filename of override header.
  string(REPLACE ".h" "_override.h" override_file ${filename})

  # Check if we have an override defined for this platform.
  # TODO: Replace by if(IN_LIST) once we bump to CMake 3.3
  if(";${ARG_PLATFORMS};" MATCHES ";${CORE_PLATFORM_NAME};")
    message(STATUS "Override active for ${filename} on ${CORE_PLATFORM_NAME}")
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${override_file} "#include \"${CMAKE_CURRENT_SOURCE_DIR}/${CORE_PLATFORM_NAME}/${filename}\"")

    # Add platform specific header to target sources (for IDEs)
    target_sources(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${CORE_PLATFORM_NAME}/${filename})
  else()
    # Issue an error if a file exists but it's not listed in add_platform_override.
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${CORE_PLATFORM_NAME}/${filename})
      message(FATAL_ERROR "Disabled platform override file detected, add it to the 'add_platform_override' call.")
    endif()

    message(STATUS "Override disabled for ${filename}, using generic implementation ${override_file}")
    string(CONCAT COMMENT "// No platform override defined for ${CORE_PLATFORM_NAME}. To add overrides:\n"
                          "// Create '${CMAKE_CURRENT_SOURCE_DIR}/${CORE_PLATFORM_NAME}/${filename}' and redefine symbols from '${filename}'.\n"
                          "// Then adapt '${CMAKE_CURRENT_LIST_FILE}' and add '${CORE_PLATFORM_NAME}' to the 'add_platform_override' call.")
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${override_file} ${COMMENT})
  endif()

  # Add generated file to target sources (for IDEs)
  target_sources(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${override_file})

  # TODO: If we want to allow the usage of the header in others headers, change to PUBLIC
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endfunction()
