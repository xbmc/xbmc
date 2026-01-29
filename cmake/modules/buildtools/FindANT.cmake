#.rst:
# FindANT
# ----------
# Finds ant executable
#
# This will define the following variables::
#
# ANT::ANT - the ANT executable

if(NOT TARGET ANT::ANT)
  include(FindPackageHandleStandardArgs)

  if(WIN32 OR WINDOWS_STORE)
    set(ant_searchname ant.bat)
  endif()

  # ANT is platform agnostic. Add hints for either native or target depends paths
  find_program(ANT_EXECUTABLE ${ant_searchname} ant
                              HINTS ${NATIVEPREFIX}/share/ant/bin
                                    ${DEPENDS_PATH}/share/ant/bin)

  if(ANT_EXECUTABLE)
    execute_process(COMMAND ${ANT_EXECUTABLE} -version
                    OUTPUT_VARIABLE ANT_VERSION
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

    string(REGEX MATCH "[^\n]* version [^\n]*" ANT_VERSION "${ANT_VERSION}")
    string(REGEX REPLACE ".* version (.*) compiled .*" "\\1" ANT_VERSION "${ANT_VERSION}")
  endif()

  # Provide standardized success/failure messages
  find_package_handle_standard_args(ANT
                                    REQUIRED_VARS ANT_EXECUTABLE
                                    VERSION_VAR ANT_VERSION)

  if(ANT_FOUND)
    # Get parent path to set custom ANT_PATH property
    get_filename_component(ANT_PATH ${ANT_EXECUTABLE} DIRECTORY)
    get_filename_component(ANT_HOME ${ANT_PATH} DIRECTORY)

    add_executable(ANT::ANT IMPORTED GLOBAL)
    set_target_properties(ANT::ANT PROPERTIES
                                       IMPORTED_LOCATION "${ANT_EXECUTABLE}"
                                       ANT_PATH "${ANT_PATH}"
                                       ANT_HOME "${ANT_HOME}"
                                       VERSION "${ANT_VERSION}")
  else()
    if(ANT_FIND_REQUIRED)
      message(FATAL_ERROR "Apache ANT was not found.")
    endif()
  endif()
endif()
