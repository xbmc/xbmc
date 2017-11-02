#.rst:
# FindTexturePacker
# -----------------
# Finds the TexturePacker
#
# If WITH_TEXTUREPACKER is defined and points to a directory,
# this path will be used to search for the Texturepacker binary
#
#
# This will define the following (imported) targets::
#
#   TexturePacker::TexturePacker   - The TexturePacker executable

if(NOT TARGET TexturePacker::TexturePacker)
  if(KODI_DEPENDSBUILD)
    add_executable(TexturePacker::TexturePacker IMPORTED GLOBAL)
    set_target_properties(TexturePacker::TexturePacker PROPERTIES
                                                       IMPORTED_LOCATION "${NATIVEPREFIX}/bin/TexturePacker")
  elseif(WIN32)
    add_executable(TexturePacker::TexturePacker IMPORTED GLOBAL)
    set_target_properties(TexturePacker::TexturePacker PROPERTIES
                                                       IMPORTED_LOCATION "${DEPENDENCIES_DIR}/tools/TexturePacker/TexturePacker.exe")
  else()
    if(WITH_TEXTUREPACKER)
      get_filename_component(_tppath ${WITH_TEXTUREPACKER} ABSOLUTE)
      find_program(TEXTUREPACKER_EXECUTABLE TexturePacker PATHS ${_tppath})

      include(FindPackageHandleStandardArgs)
      find_package_handle_standard_args(TexturePacker DEFAULT_MSG TEXTUREPACKER_EXECUTABLE)
      if(TEXTUREPACKER_FOUND)
        add_executable(TexturePacker::TexturePacker IMPORTED GLOBAL)
        set_target_properties(TexturePacker::TexturePacker PROPERTIES
                                                           IMPORTED_LOCATION "${TEXTUREPACKER_EXECUTABLE}")
      endif()
      mark_as_advanced(TEXTUREPACKER)
    else()
      add_subdirectory(${CMAKE_SOURCE_DIR}/tools/depends/native/TexturePacker build/texturepacker)
      add_executable(TexturePacker::TexturePacker ALIAS TexturePacker)
    endif()
  endif()
endif()
