MACRO(ADD_MSVC_PRECOMPILED_HEADER PrecompiledHeader PrecompiledSource SourcesVar)
  IF(MSVC)
    GET_FILENAME_COMPONENT(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
    SET(PrecompiledBinary "${PrecompiledBasename}.pch")
    SET(Sources ${${SourcesVar}})

    SET_SOURCE_FILES_PROPERTIES(${PrecompiledSource}
                                PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_OUTPUTS "${PrecompiledBinary}")

    # Collect all CPP sources
    foreach(src ${Sources})
      get_filename_component(SRCEXT ${src} EXT)
      if(${SRCEXT} STREQUAL ".cpp")
        list(APPEND CXX_SRCS ${src})
      endif()
    endforeach()

    SET_SOURCE_FILES_PROPERTIES(${CXX_SRCS}
                                PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_DEPENDS "${PrecompiledBinary}")  
    # Add precompiled header to SourcesVar
    LIST(APPEND ${SourcesVar} ${PrecompiledSource})
  ENDIF(MSVC)
ENDMACRO(ADD_MSVC_PRECOMPILED_HEADER)

# Replacement for aux_source_directory that also adds all headers
macro(find_all_sources DIRECTORY VARIABLE)
  aux_source_directory(${DIRECTORY} ${VARIABLE})
  file(GLOB headers ${DIRECTORY}/*h)
  list(APPEND ${VARIABLE} ${headers})
endmacro()

# function to collect all the sources from sub-directories
# into a single list
function(add_sources)
  get_property(is_defined GLOBAL PROPERTY SRCS_LIST DEFINED)
  if(NOT is_defined)
    define_property(GLOBAL PROPERTY SRCS_LIST
      BRIEF_DOCS "List of source files"
      FULL_DOCS "List of source files to be compiled in one library")
  endif()
  # make absolute paths
  set(SRCS)
  foreach(s IN LISTS ARGN)
    if(NOT IS_ABSOLUTE "${s}")
      get_filename_component(s "${s}" ABSOLUTE)
    endif()
    list(APPEND SRCS "${s}")
  endforeach()

  string(REPLACE ${CMAKE_SOURCE_DIR}/xbmc/ "" SUBDIR ${CMAKE_CURRENT_SOURCE_DIR})
  string(TOLOWER ${SUBDIR} SUBDIR)
  string(REPLACE "/" "\\" LIBNAME ${SUBDIR})
  source_group(${LIBNAME} FILES ${SRCS})

  # add it to the global list.
  set_property(GLOBAL APPEND PROPERTY SRCS_LIST ${SRCS})
endfunction(add_sources)

# function to add a test case
function(plex_add_testcase)
  if(ENABLE_TESTING)
    get_property(is_defined GLOBAL PROPERTY PLEX_TEST_CASES DEFINED)
    if(NOT is_defined)
      define_property(GLOBAL PROPERTY PLEX_TEST_CASES BRIEF_DOCS "testcases" FULL_DOCS "testcases")
    endif(NOT is_defined)

    set(TESTCASES)
    foreach(CASE IN LISTS ARGN)
      get_filename_component(CASE_FILE "${CASE}" ABSOLUTE)
      get_filename_component(CASE_BIN "${CASE}" NAME_WE)
      list(APPEND TESTCASES "${CASE_BIN}")

      include_directories(${root}/lib/gtest ${root}/lib/gtest/include)
      add_library(${CASE_BIN} OBJECT ${CASE_FILE})
    endforeach()

    set_property(GLOBAL APPEND PROPERTY PLEX_TEST_CASES ${TESTCASES})
  endif(ENABLE_TESTING)
endfunction(plex_add_testcase)

macro(plex_get_soname sonamevar library)
      # split out the library name
      get_filename_component(REALNAME ${library} REALPATH)
      get_filename_component(${sonamevar} ${REALNAME} NAME)
endmacro()

set(CMAKE_MODULE_PATH ${CMAKE_ROOT}/Modules ${CMAKE_MODULE_PATH})

# function to find library and set the variables we need
macro(plex_find_library lib framework nodefaultpath searchpath addtolinklist)
  string(TOUPPER ${lib} LIBN)
  
  if(CONFIG_LIBRARY_${LIBN})
    set(QUIET_FIND 1)
  endif()

  # find the library, just searching in our paths
  if(${nodefaultpath})
    find_library(CONFIG_LIBRARY_${LIBN} ${lib} PATHS ${searchpath} ${searchpath}64 NO_DEFAULT_PATH)
  else()
    find_library(CONFIG_LIBRARY_${LIBN} ${lib} PATHS ${searchpath})
  endif()

  if(CONFIG_LIBRARY_${LIBN} MATCHES "NOTFOUND")
      message(FATAL_ERROR "Could not detect ${LIBN}")
  else()
      # get the actual value
      get_property(l VARIABLE PROPERTY CONFIG_LIBRARY_${LIBN})
      
      # resolve any symlinks
      get_filename_component(REALNAME ${l} REALPATH)

      # split out the library name
      get_filename_component(FNAME ${REALNAME} NAME)
      
      # set the SONAME variable, needed for DllPaths_generated.h
      set(${LIBN}_SONAME ${FNAME} CACHE string "the soname for the current library")
      set(LIB${LIBN}_SONAME ${FNAME} CACHE string "the soname for the current library")
      
      # set the HAVE_LIBX variable
      set(HAVE_LIB${LIBN} 1 CACHE string "the HAVE_LIBX variable")
      
      # if this is a framework we need to mark it as advanced
      if(${framework})
        mark_as_advanced(CONFIG_LIBRARY_${LIBN})
      endif()
      
      if(${addtolinklist})
        list(APPEND CONFIG_PLEX_LINK_LIBRARIES ${l})
      else()
        list(APPEND CONFIG_PLEX_INSTALL_LIBRARIES ${REALNAME})
      endif()
      
      if(NOT QUIET_FIND)
        message(STATUS "Looking for library ${lib} - found")
      endif()
  endif()
endmacro()

macro(plex_find_package package required addtolinklist)
  if(${required})
    find_package(${package} REQUIRED)
  else(${required})
    find_package(${package})
  endif(${required})
  
  string(TOUPPER ${package} PKG_UPPER_NAME)
  string(REPLACE "_" "" PKG_NAME ${PKG_UPPER_NAME})
  string(REGEX REPLACE "^LIB" "" PKG_NO_LIB_NAME ${PKG_UPPER_NAME})
  
  if(${PKG_NAME}_FOUND)
    if(${PKG_NAME}_INCLUDE_DIR)
      get_property(PKG_INC VARIABLE PROPERTY ${PKG_NAME}_INCLUDE_DIR)
    else()
      get_property(PKG_INC VARIABLE PROPERTY ${PKG_NAME}_INCLUDE_DIRS)
    endif()

    include_directories(${PKG_INC})

    if(${PKG_NAME}_LIBRARIES)
      get_property(PKG_LIB VARIABLE PROPERTY ${PKG_NAME}_LIBRARIES)
    else()
      get_property(PKG_LIB VARIABLE PROPERTY ${PKG_NAME}_LIBRARY)
    endif()

    set(CONFIG_LIBRARY_${PKG_UPPER_NAME} ${PKG_LIB})

    if(${addtolinklist})
      list(APPEND CONFIG_PLEX_LINK_LIBRARIES ${PKG_LIB})
    else()
      list(APPEND CONFIG_PLEX_INSTALL_LIBRARIES ${PKG_LIB})
    endif()

    set(HAVE_LIB${PKG_NO_LIB_NAME} 1 CACHE string "if this lib is around or not")
  else()
    if(${required})
      message(FATAL_ERROR "Missing ${PKG_NAME}")
    endif()
  endif()
endmacro()

macro(plex_find_headers) 
  set(args HEADERS HINTS)
  include(CMakeParseArguments)
  cmake_parse_arguments(FIND_HEADERS "" "" "${args}" ${ARGN})
  foreach(h ${FIND_HEADERS_HEADERS})
    plex_find_header(${h} "${FIND_HEADERS_HINTS}")
  endforeach()
endmacro()

macro(plex_find_header header hintpath)
  set(_HAVE_VAR HAVE_${header}_H)
  string(TOUPPER ${_HAVE_VAR} _HAVE_VAR)
  string(REPLACE "/" "_" _HAVE_VAR ${_HAVE_VAR})
  string(REPLACE ".H" "" _HAVE_VAR ${_HAVE_VAR})
  
  if(DEFINED ${_HAVE_VAR})
    set(QUIET_FIND 1)
  endif()
  
  find_path(_${_HAVE_VAR} NAMES ${header} HINTS ${hintpath})
  get_property(v VARIABLE PROPERTY _${_HAVE_VAR})

  if(NOT v MATCHES "NOTFOUND")
    set(${_HAVE_VAR} 1 CACHE STRING "Have this header?")
    if(NOT QUIET_FIND)
      message(STATUS "Looking for include file ${header} - found")
    endif()
  else()
    if(NOT QUIET_FIND)
      message(STATUS "Looking for include file ${header} - not found")
    endif()
  endif()
  
endmacro()
