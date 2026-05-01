# Helper functions for applying patches in ExternalProject patch steps.

get_filename_component(_patchhelpers_apply_patches_script
                       ${CMAKE_CURRENT_LIST_DIR}/../../tools/depends/apply-patches.sh
                       ABSOLUTE)

# Macro to test format of line endings of a patch.
# Windows specific.
macro(PATCH_LF_CHECK patch)
  if(CMAKE_HOST_WIN32)
    # On Windows "patch.exe" can only handle CR-LF line-endings.
    # Our patches have LF-only line endings - except when they
    # have been checked out as part of a dependency hosted on Git
    # and core.autocrlf=true.
    file(READ ${ARGV0} patch_content_hex HEX)
    # Force handle LF-only line endings
    if(NOT patch_content_hex MATCHES "0d0a")
      if (NOT "--binary" IN_LIST PATCH_EXECUTABLE)
        list(APPEND PATCH_EXECUTABLE --binary)
      endif()
    else()
      if ("--binary" IN_LIST PATCH_EXECUTABLE)
        list(REMOVE_ITEM PATCH_EXECUTABLE --binary)
      endif()
    endif()
  endif()
  unset(patch_content_hex)
endmacro()

# Function to loop through list of patch files (full path).
# Sets the generated command in the parent scope.
function(generate_patchcommand _patchlist)
  set(oneValueArgs OUTPUT_VARIABLE)
  cmake_parse_arguments(arg "" "${oneValueArgs}" "" ${ARGN})
  if(NOT arg_OUTPUT_VARIABLE)
    set(arg_OUTPUT_VARIABLE PATCH_COMMAND)
  endif()

  # find the path to the patch executable
  find_package(Patch MODULE REQUIRED ${SEARCH_QUIET})

  if(WIN32 OR WINDOWS_STORE)
    # Loop through patches and add to PATCH_COMMAND.
    # For Windows, check CRLF/LF state.
    set(_count 0)
    foreach(patch ${_patchlist})
      PATCH_LF_CHECK(${patch})
      if(${_count} EQUAL "0")
        set(_patch_command ${PATCH_EXECUTABLE} -p1 -i ${patch})
      else()
        list(APPEND _patch_command COMMAND ${PATCH_EXECUTABLE} -p1 -i ${patch})
      endif()

      math(EXPR _count "${_count}+1")
    endforeach()
    unset(_count)
  else()
    if(EXISTS "${_patchhelpers_apply_patches_script}")
      set(_apply_patches_script ${_patchhelpers_apply_patches_script})
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/apply-patches.sh")
      set(_apply_patches_script ${CMAKE_SOURCE_DIR}/apply-patches.sh)
    else()
      message(FATAL_ERROR "Could not find apply-patches.sh")
    endif()

    set(_patch_command /bin/sh
                       ${_apply_patches_script}
                       ${PATCH_EXECUTABLE}
                       <SOURCE_DIR>
                       ${_patchlist})
    unset(_apply_patches_script)
  endif()

  set(${arg_OUTPUT_VARIABLE} ${_patch_command} PARENT_SCOPE)
  unset(_patch_command)
endfunction()
