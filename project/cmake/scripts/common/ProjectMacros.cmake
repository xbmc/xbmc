# This script holds macros which are project specific

# Pack a skin xbt file
# Arguments:
#   input  input directory to pack
#   output ouput xbt file
# On return:
#   xbt is added to ${XBT_FILES}
function(pack_xbt input output)
  file(GLOB_RECURSE MEDIA_FILES ${input}/*)
  get_filename_component(dir ${output} PATH)
  add_custom_command(OUTPUT  ${output}
                     COMMAND ${CMAKE_COMMAND} -E make_directory ${dir}
                     COMMAND TexturePacker::TexturePacker
                     ARGS    -input ${input}
                             -output ${output}
                             -dupecheck
                     DEPENDS ${MEDIA_FILES})
  list(APPEND XBT_FILES ${output})
  set(XBT_FILES ${XBT_FILES} PARENT_SCOPE)
endfunction()

# Add a skin to installation list, mirroring it in build tree, packing textures
# Arguments:
#   skin     skin directory
# On return:
#   xbt is added to ${XBT_FILES}, data added to ${install_data}, mirror in build tree
function(copy_skin_to_buildtree skin)
  file(GLOB_RECURSE FILES ${skin}/*)
  file(GLOB_RECURSE MEDIA_FILES ${skin}/media/*)
  list(REMOVE_ITEM FILES ${MEDIA_FILES})
  foreach(file ${FILES})
    copy_file_to_buildtree(${file})
  endforeach()
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${dest}/media)
  string(REPLACE "${CORE_SOURCE_DIR}/" "" dest ${skin})
  pack_xbt(${skin}/media ${CMAKE_BINARY_DIR}/${dest}/media/Textures.xbt)

  file(GLOB THEMES RELATIVE ${skin}/themes ${skin}/themes/*)
  foreach(theme ${THEMES})
    pack_xbt(${skin}/themes/${theme} ${CMAKE_BINARY_DIR}/${dest}/media/${theme}.xbt)
  endforeach()

  set(XBT_FILES ${XBT_FILES} PARENT_SCOPE)
  set(install_data ${install_data} PARENT_SCOPE)
endfunction()

# Get GTest tests as CMake tests.
# Copied from FindGTest.cmake 
# Thanks to Daniel Blezek <blezek@gmail.com> for the GTEST_ADD_TESTS code
function(GTEST_ADD_TESTS executable extra_args)
    if(NOT ARGN)
        message(FATAL_ERROR "Missing ARGN: Read the documentation for GTEST_ADD_TESTS")
    endif()
    foreach(source ${ARGN})
        # This assumes that every source file passed in exists. Consider using
        # SUPPORT_SOURCES for source files which do not contain tests and might
        # have to be generated.
        file(READ "${source}" contents)
        string(REGEX MATCHALL "TEST_?[F]?\\(([A-Za-z_0-9 ,]+)\\)" found_tests ${contents})
        foreach(hit ${found_tests})
            string(REGEX REPLACE ".*\\( *([A-Za-z_0-9]+), *([A-Za-z_0-9]+) *\\).*" "\\1.\\2" test_name ${hit})
            add_test(${test_name} ${executable} --gtest_filter=${test_name} ${extra_args})
        endforeach()
        # Groups parametrized tests under a single ctest entry
        string(REGEX MATCHALL "INSTANTIATE_TEST_CASE_P\\(([^,]+), *([^,]+)" found_tests2 ${contents})
        foreach(hit ${found_tests2})
          string(SUBSTRING ${hit} 24 -1 test_name)
          string(REPLACE "," ";" test_name "${test_name}")
          list(GET test_name 0 filter_name)
          list(GET test_name 1 test_prefix)
          string(STRIP ${test_prefix} test_prefix)
          add_test(${test_prefix}.${filter_name} ${executable} --gtest_filter=${filter_name}* ${extra_args})
        endforeach()
    endforeach()
endfunction()

function(whole_archive output)
  if(CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    set(${output} -Wl,--whole-archive ${ARGN} -Wl,--no-whole-archive PARENT_SCOPE)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL AppleClang)
    foreach(library ${ARGN})
      list(APPEND ${output} -Wl,-force_load ${library})
      set(${output} ${${output}} PARENT_SCOPE)
    endforeach()
  else()
    set(${output} ${ARGN} PARENT_SCOPE)
  endif()
endfunction()
