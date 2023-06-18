#.rst:
# FindTexturePacker
# -----------------
# Finds TexturePacker executable
#
# If WITH_TEXTUREPACKER is defined and points to a directory,
# this path will be used to search for the Texturepacker binary
#
# This will define the following target:
#
#   TexturePacker::TexturePacker - The TexturePacker executable

if(NOT TARGET TexturePacker::TexturePacker)
  if(WITH_TEXTUREPACKER)
    get_filename_component(_tppath ${WITH_TEXTUREPACKER} ABSOLUTE)
    get_filename_component(_tppath ${_tppath} DIRECTORY)
    find_program(TEXTUREPACKER_EXECUTABLE NAMES "${APP_NAME_LC}-TexturePacker" TexturePacker
                                                "${APP_NAME_LC}-TexturePacker.exe" TexturePacker.exe
                                          HINTS ${_tppath}
                                          NO_CACHE)

    if(NOT TEXTUREPACKER_EXECUTABLE)
      message(FATAL_ERROR "Could not find 'TexturePacker' executable in ${_tppath} supplied by -DWITH_TEXTUREPACKER")
    endif()
  else()
    include(cmake/scripts/common/ModuleHelpers.cmake)

    # Check for existing TEXTUREPACKER
    find_program(TEXTUREPACKER_EXECUTABLE NAMES "${APP_NAME_LC}-TexturePacker" TexturePacker
                                                "${APP_NAME_LC}-TexturePacker.exe" TexturePacker.exe
                                          HINTS ${NATIVEPREFIX}/bin
                                          NO_CACHE)

    if(TEXTUREPACKER_EXECUTABLE)
      execute_process(COMMAND "${TEXTUREPACKER_EXECUTABLE}" -version
                      OUTPUT_VARIABLE TEXTUREPACKER_EXECUTABLE_VERSION
                      OUTPUT_STRIP_TRAILING_WHITESPACE)
      string(REGEX MATCH "[^\n]* version [^\n]*" TEXTUREPACKER_EXECUTABLE_VERSION "${TEXTUREPACKER_EXECUTABLE_VERSION}")
      string(REGEX REPLACE ".* version (.*)" "\\1" TEXTUREPACKER_EXECUTABLE_VERSION "${TEXTUREPACKER_EXECUTABLE_VERSION}")
    endif()

    set(MODULE_LC TexturePacker)
    set(LIB_TYPE native)
    SETUP_BUILD_VARS()

    if(NOT TEXTUREPACKER_EXECUTABLE OR "${TEXTUREPACKER_EXECUTABLE_VERSION}" VERSION_LESS "${TEXTUREPACKER_VER}")

      # Override build type detection and always build as release
      set(TEXTUREPACKER_BUILD_TYPE Release)

      set(CMAKE_ARGS -DKODI_SOURCE_DIR=${CMAKE_SOURCE_DIR})

      if(NATIVEPREFIX)
        set(INSTALL_DIR "${NATIVEPREFIX}/bin")
        set(TEXTUREPACKER_INSTALL_PREFIX ${NATIVEPREFIX})
        list(APPEND CMAKE_ARGS "-DNATIVEPREFIX=${NATIVEPREFIX}")
      else()
        set(INSTALL_DIR "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/bin")
        set(TEXTUREPACKER_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR})
      endif()

      # Set host build info for buildtool
      if(EXISTS "${NATIVEPREFIX}/share/Toolchain-Native.cmake")
        set(TEXTUREPACKER_TOOLCHAIN_FILE "${NATIVEPREFIX}/share/Toolchain-Native.cmake")
        list(APPEND CMAKE_ARGS -DENABLE_STATIC=1)
      else()
        if(WIN32)
          # Windows ARCH_DEFINES for store has some things we probably dont want
          # just provide a simple set of defines for all windows host builds
          list(APPEND CMAKE_ARGS -DARCH_DEFINES=TARGET_WINDOWS;WIN32;_CONSOLE;_CRT_SECURE_NO_WARNINGS)
          list(APPEND CMAKE_ARGS -DENABLE_STATIC=1)
        else()
          string(REGEX REPLACE "-D" "" STRIPPED_ARCH_DEFINES "${ARCH_DEFINES}")
          list(APPEND CMAKE_ARGS "-DARCH_DEFINES=${STRIPPED_ARCH_DEFINES}")
        endif()
      endif()

      if(WIN32 OR WINDOWS_STORE)
        # Make sure we generate for host arch, not target
        set(TEXTUREPACKER_GENERATOR_PLATFORM CMAKE_GENERATOR_PLATFORM WIN32)
        set(WIN_DISABLE_PROJECT_FLAGS 1)
        set(APP_EXTENSION ".exe")
      endif()

      set(TEXTUREPACKER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/tools/depends/native/TexturePacker/src)
      set(TEXTUREPACKER_EXECUTABLE ${INSTALL_DIR}/TexturePacker${APP_EXTENSION})
      set(TEXTUREPACKER_EXECUTABLE_VERSION ${TEXTUREPACKER_VER})

      set(BUILD_BYPRODUCTS ${TEXTUREPACKER_EXECUTABLE})

      BUILD_DEP_TARGET()
    endif()
  endif()

  include(FindPackageMessage)
  find_package_message(TexturePacker "Found TexturePacker: ${TEXTUREPACKER_EXECUTABLE} (found version \"${TEXTUREPACKER_EXECUTABLE_VERSION}\")"
                                     "[${TEXTUREPACKER_EXECUTABLE}][${TEXTUREPACKER_EXECUTABLE_VERSION}]")

  add_executable(TexturePacker::TexturePacker IMPORTED GLOBAL)
  set_target_properties(TexturePacker::TexturePacker PROPERTIES
                                                     IMPORTED_LOCATION "${TEXTUREPACKER_EXECUTABLE}")

  if(TARGET TexturePacker)
    add_dependencies(TexturePacker::TexturePacker TexturePacker)
  endif()
endif()
