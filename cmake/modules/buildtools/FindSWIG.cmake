#.rst:
# FindSWIG
# --------
# Finds the SWIG executable
#
# This will define the following variables::
#
# SWIG_FOUND - system has SWIG
# SWIG_EXECUTABLE - the SWIG executable

find_program(SWIG_EXECUTABLE NAMES swig4.0 swig3.0 swig2.0 swig
                             PATH_SUFFIXES swig
                             HINTS ${NATIVEPREFIX}/bin)
if(SWIG_EXECUTABLE)
  execute_process(COMMAND ${SWIG_EXECUTABLE} -swiglib
    OUTPUT_VARIABLE SWIG_DIR
    ERROR_VARIABLE SWIG_swiglib_error
    RESULT_VARIABLE SWIG_swiglib_result)
  execute_process(COMMAND ${SWIG_EXECUTABLE} -version
    OUTPUT_VARIABLE SWIG_version_output
    ERROR_VARIABLE SWIG_version_output
    RESULT_VARIABLE SWIG_version_result)
    string(REGEX REPLACE ".*SWIG Version[^0-9.]*\([0-9.]+\).*" "\\1"
           SWIG_VERSION "${SWIG_version_output}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SWIG
                                  REQUIRED_VARS SWIG_EXECUTABLE SWIG_DIR
                                  VERSION_VAR SWIG_VERSION)
