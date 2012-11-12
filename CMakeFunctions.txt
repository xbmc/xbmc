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
