#.rst:
# FindSWIG
# --------
# Finds the SWIG executable
#
# This will define the following TARGET:
#
# SWIG::SWIG - the SWIG executable

if(NOT TARGET SWIG::SWIG)
  find_program(SWIG_EXECUTABLE NAMES swig4.0 swig3.0 swig2.0 swig
                               NO_CACHE
                               PATH_SUFFIXES swig
                               HINTS ${NATIVEPREFIX}/bin)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(SWIG
                                    REQUIRED_VARS SWIG_EXECUTABLE
                                    VERSION_VAR SWIG_VERSION)

  if(SWIG_FOUND)
    execute_process(COMMAND ${SWIG_EXECUTABLE} -version
      OUTPUT_VARIABLE SWIG_version_output
      ERROR_VARIABLE SWIG_version_output
      RESULT_VARIABLE SWIG_version_result)
      string(REGEX REPLACE ".*SWIG Version[^0-9.]*\([0-9.]+\).*" "\\1"
             SWIG_VERSION "${SWIG_version_output}")

    add_executable(SWIG::SWIG IMPORTED GLOBAL)
    set_target_properties(SWIG::SWIG PROPERTIES
                                     IMPORTED_LOCATION "${SWIG_EXECUTABLE}"
                                     VERSION "${SWIG_VERSION}")
  else()
    if(SWIG_FIND_REQUIRED)
      message(FATAL_ERROR "Swig executable was not found.")
    endif()
  endif()
endif()
