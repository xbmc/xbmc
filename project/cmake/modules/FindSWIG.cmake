if(NOT SWIG_EXECUTABLE)
  find_program(SWIG_EXECUTABLE NAMES swig2.0 swig PATH_SUFFIXES swig)
endif()
if(SWIG_EXECUTABLE)
  execute_process(COMMAND ${SWIG_EXECUTABLE} -swiglib
    OUTPUT_VARIABLE SWIG_DIR
    ERROR_VARIABLE SWIG_swiglib_error
    RESULT_VARIABLE SWIG_swiglib_result)
endif()


include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SWIG  REQUIRED_VARS SWIG_EXECUTABLE SWIG_DIR
                                        VERSION_VAR SWIG_VERSION )
