#.rst:
# FindMeson
# ----------
# Finds meson executable
#
# This will define the following variables::
#
# Meson::Meson - the Meson executable

if(NOT TARGET Meson::Meson)
  include(FindPackageHandleStandardArgs)

  find_program(MESON_EXECUTABLE meson
                                HINTS ${NATIVEPREFIX}/bin
                                      # windows tools package path has meson/ninja bundled
                                      ${NATIVEPREFIX}/bin/Meson)

  if(MESON_EXECUTABLE)
    execute_process(COMMAND ${MESON_EXECUTABLE} -version
                    OUTPUT_VARIABLE MESON_VERSION
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()

  # Provide standardized success/failure messages
  find_package_handle_standard_args(Meson
                                    REQUIRED_VARS MESON_EXECUTABLE
                                    VERSION_VAR MESON_VERSION)

  if(Meson_FOUND)
    add_executable(Meson::Meson IMPORTED GLOBAL)
    set_target_properties(Meson::Meson PROPERTIES
                                       IMPORTED_LOCATION "${MESON_EXECUTABLE}"
                                       VERSION "${MESON_VERSION}")

    # For now, we only have to generate a cross file for windows platforms
    # tools/depends platforms all create one as part of their configure
    if(WIN32 OR WINDOWS_STORE)
      if(KODI_SOURCE_DIR)
        set(base_path ${KODI_SOURCE_DIR}/)
      endif()

      include(${base_path}cmake/scripts/common/ModuleHelpers.cmake)
      generate_mesoncrossfile()
    endif()
  else()
    if(Meson_FIND_REQUIRED)
      message(FATAL_ERROR "Meson was not found.")
    endif()
  endif()
endif()
