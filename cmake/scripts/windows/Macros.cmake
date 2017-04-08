function(core_link_library lib wraplib)
  message(AUTHOR_WARNING "core_link_library is not compatible with windows.")
endfunction()

function(find_soname lib)
  # Windows uses hardcoded dlls in xbmc/DllPaths_win32.h.
  # Therefore the output of this function is unused.
endfunction()

# Add precompiled header to target
# Arguments:
#   target existing target that will be set up to compile with a precompiled header
#   pch_header the precompiled header file
#   pch_source the precompiled header source file
# Optional Arguments:
#   PCH_TARGET build precompiled header as separate target with the given name
#              so that the same precompiled header can be used for multiple libraries
#   EXCLUDE_SOURCES if not all target sources shall use the precompiled header,
#                   the relevant files can be listed here
# On return:
#   Compiles the pch_source into a precompiled header and adds the header to
#   the given target
function(add_precompiled_header target pch_header pch_source)
  cmake_parse_arguments(PCH "" "PCH_TARGET" "EXCLUDE_SOURCES" ${ARGN})

  if(PCH_PCH_TARGET)
    set(pch_binary ${PRECOMPILEDHEADER_DIR}/${PCH_PCH_TARGET}.pch)
  else()
    set(pch_binary ${PRECOMPILEDHEADER_DIR}/${target}.pch)
  endif()

  # Set compile options and dependency for sources
  get_target_property(sources ${target} SOURCES)
  list(REMOVE_ITEM sources ${pch_source})
  foreach(exclude_source IN LISTS PCH_EXCLUDE_SOURCES)
    list(REMOVE_ITEM sources ${exclude_source})
  endforeach()
  set_source_files_properties(${sources}
                              PROPERTIES COMPILE_FLAGS "/Yu\"${pch_header}\" /Fp\"${pch_binary}\" /FI\"${pch_header}\""
                              OBJECT_DEPENDS "${pch_binary}")

  # Set compile options for precompiled header
  if(NOT PCH_PCH_TARGET OR NOT TARGET ${PCH_PCH_TARGET}_pch)
    set_source_files_properties(${pch_source}
                                PROPERTIES COMPILE_FLAGS "/Yc\"${pch_header}\" /Fp\"${pch_binary}\""
                                OBJECT_OUTPUTS "${pch_binary}")
  endif()

  # Compile precompiled header
  if(PCH_PCH_TARGET)
    # As own target for usage in multiple libraries
    if(NOT TARGET ${PCH_PCH_TARGET}_pch)
      add_library(${PCH_PCH_TARGET}_pch STATIC ${pch_source})
      set_target_properties(${PCH_PCH_TARGET}_pch PROPERTIES COMPILE_PDB_NAME vc140
                                                             COMPILE_PDB_OUTPUT_DIRECTORY ${PRECOMPILEDHEADER_DIR}
                                                             FOLDER "Build Utilities")
    endif()
    # From VS2012 onwards, precompiled headers have to be linked against (LNK2011).
    target_link_libraries(${target} PUBLIC ${PCH_PCH_TARGET}_pch)
    set_target_properties(${target} PROPERTIES COMPILE_PDB_NAME vc140
                                               COMPILE_PDB_OUTPUT_DIRECTORY ${PRECOMPILEDHEADER_DIR})
  else()
    # As part of the target
    target_sources(${target} PRIVATE ${pch_source})
  endif()
endfunction()
