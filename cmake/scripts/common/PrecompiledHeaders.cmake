# Precompiled headers for non-Windows platforms. Windows sets up its own in core_add_library.
#
# On include:
#   CORE_USE_PCH is set to whether precompiled headers are used

if(ENABLE_PCH AND ENABLE_STATIC_LIBS AND NOT WIN32)
  set(CORE_USE_PCH TRUE)
else()
  set(CORE_USE_PCH FALSE)
endif()

# Adds the compile options that every target using a PCH needs
function(core_pch_compile_options target)
  # Keeps a PCH identical when its headers are only touched, so ccache can reuse what uses it
  if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    target_compile_options(${target} PRIVATE "SHELL:-Xclang -fno-pch-timestamp")
  endif()
endfunction()

# Gives a target a PCH of its own
function(core_target_precompile_headers target)
  core_pch_compile_options(${target})
  target_precompile_headers(${target} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/xbmc/platform/posix/pch.h>)
endfunction()

# Sets up the PCH of all core libraries. The ones without flags of their own share one.
function(core_add_precompiled_headers)
  set(owner ${APP_NAME_LC}_pch)

  # Set up like a core library. CMake only creates the PCH of a target that has a source file to
  # use it with.
  file(CONFIGURE OUTPUT ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/pch_dummy.cpp CONTENT "")
  add_library(${owner} OBJECT ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/pch_dummy.cpp)
  core_target_link_libraries(${owner})
  target_compile_options(${owner} PUBLIC ${CORE_COMPILE_OPTIONS})
  core_target_precompile_headers(${owner})

  get_directory_property(owner_directory_definitions COMPILE_DEFINITIONS)
  set(properties COMPILE_OPTIONS COMPILE_DEFINITIONS POSITION_INDEPENDENT_CODE LINK_LIBRARIES)
  foreach(property IN LISTS properties)
    get_property(owner_${property} TARGET ${owner} PROPERTY ${property})
  endforeach()

  foreach(library IN LISTS core_DEPENDS)
    get_target_property(type ${library} TYPE)
    if(NOT type STREQUAL STATIC_LIBRARY)
      continue()
    endif()

    core_pch_compile_options(${library})

    # A PCH can only be used with the flags it was built with
    get_target_property(source_dir ${library} SOURCE_DIR)
    get_directory_property(directory_definitions DIRECTORY ${source_dir} COMPILE_DEFINITIONS)
    if("${directory_definitions}" STREQUAL "${owner_directory_definitions}")
      set(matches TRUE)
    else()
      set(matches FALSE)
    endif()
    foreach(property IN LISTS properties)
      get_property(value TARGET ${library} PROPERTY ${property})
      if(NOT "${value}" STREQUAL "${owner_${property}}")
        set(matches FALSE)
      endif()
    endforeach()

    # Reusing a PCH requires one for each language of the library, the shared one is C++ only
    get_target_property(sources ${library} SOURCES)
    list(FILTER sources INCLUDE REGEX "\\.(c|m|mm)$")
    if(sources)
      set(matches FALSE)
    endif()

    if(matches)
      target_precompile_headers(${library} REUSE_FROM ${owner})
    else()
      core_target_precompile_headers(${library})
      list(APPEND own_pch_libraries ${library})
    endif()
  endforeach()

  if(VERBOSE)
    message(STATUS "Libraries with their own PCH: ${own_pch_libraries}")
  endif()
endfunction()
