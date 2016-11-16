#.rst:
# FindTexturePacker
# -----------------
# Finds the TexturePacker
#
# This will define the following (imported) targets::
#
#   TexturePacker::TexturePacker   - The TexturePacker executable

if(NOT TARGET TexturePacker::TexturePacker)
  if(CMAKE_CROSSCOMPILING)
    add_executable(TexturePacker::TexturePacker IMPORTED GLOBAL)
    set_target_properties(TexturePacker::TexturePacker PROPERTIES
                                                       IMPORTED_LOCATION "${NATIVEPREFIX}/bin/TexturePacker")
  elseif(WIN32)
    add_executable(TexturePacker::TexturePacker IMPORTED GLOBAL)
    set_target_properties(TexturePacker::TexturePacker PROPERTIES
                                                       IMPORTED_LOCATION "${CORE_SOURCE_DIR}/tools/TexturePacker/TexturePacker.exe")
  else()
    add_subdirectory(${CORE_SOURCE_DIR}/tools/depends/native/TexturePacker build/texturepacker)
    add_executable(TexturePacker::TexturePacker ALIAS TexturePacker)
  endif()
endif()
