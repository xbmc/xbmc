#.rst:
# FindNinja
# ----------
# Finds ninja executable
#
# This will define the following variables::
#
# Ninja::Ninja - the Ninja executable

if(NOT TARGET Ninja::Ninja)
  include(FindPackageHandleStandardArgs)

  find_program(NINJA_EXECUTABLE ninja
                                HINTS ${NATIVEPREFIX}/bin
                                      # windows tools package path has meson/ninja bundled
                                      ${NATIVEPREFIX}/bin/Meson)

  if(NINJA_EXECUTABLE)
    execute_process(COMMAND ${NINJA_EXECUTABLE} --version
                    OUTPUT_VARIABLE NINJA_VERSION
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()

  # Provide standardized success/failure messages
  find_package_handle_standard_args(Ninja
                                    REQUIRED_VARS NINJA_EXECUTABLE
                                    VERSION_VAR NINJA_VERSION)

  if(Ninja_FOUND)
    add_executable(Ninja::Ninja IMPORTED GLOBAL)
    set_target_properties(Ninja::Ninja PROPERTIES
                                       IMPORTED_LOCATION "${NINJA_EXECUTABLE}"
                                       VERSION "${NINJA_VERSION}")
  else()
    if(Ninja_FIND_REQUIRED)
      message(FATAL_ERROR "Ninja was not found.")
    endif()
  endif()
endif()
