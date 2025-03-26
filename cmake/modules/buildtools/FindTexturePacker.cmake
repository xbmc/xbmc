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
#   TexturePacker::TexturePacker::Executable   - The TexturePacker executable participating in build
#   TexturePacker::TexturePacker::Installable  - The TexturePacker executable shipped in the Kodi package

if(NOT TARGET TexturePacker::TexturePacker::Executable)
  if(KODI_DEPENDSBUILD)
    get_filename_component(_tppath "${NATIVEPREFIX}/bin" ABSOLUTE)
    find_program(TEXTUREPACKER_EXECUTABLE NAMES "${APP_NAME_LC}-TexturePacker" TexturePacker
                                          HINTS ${_tppath})

    add_executable(TexturePacker::TexturePacker::Executable IMPORTED GLOBAL)
    set_target_properties(TexturePacker::TexturePacker::Executable PROPERTIES
                                          IMPORTED_LOCATION "${TEXTUREPACKER_EXECUTABLE}")
    message(STATUS "External TexturePacker for KODI_DEPENDSBUILD will be executed during build: ${TEXTUREPACKER_EXECUTABLE}")
  elseif(WIN32)
    get_filename_component(_tppath "${NATIVEPREFIX}/tools/TexturePacker" ABSOLUTE)
    find_program(TEXTUREPACKER_EXECUTABLE NAMES "${APP_NAME_LC}-TexturePacker.exe" TexturePacker.exe
                                          HINTS ${_tppath})

    add_executable(TexturePacker::TexturePacker::Executable IMPORTED GLOBAL)
    set_target_properties(TexturePacker::TexturePacker::Executable PROPERTIES
                                          IMPORTED_LOCATION "${TEXTUREPACKER_EXECUTABLE}")
    message(STATUS "External TexturePacker for WIN32 will be executed during build: ${TEXTUREPACKER_EXECUTABLE}")
  else()
    if(WITH_TEXTUREPACKER)
      get_filename_component(_tppath ${WITH_TEXTUREPACKER} ABSOLUTE)
      get_filename_component(_tppath ${_tppath} DIRECTORY)
      find_program(TEXTUREPACKER_EXECUTABLE NAMES "${APP_NAME_LC}-TexturePacker" TexturePacker
                                          HINTS ${_tppath})

      # Use external TexturePacker executable if found
      if(TEXTUREPACKER_EXECUTABLE)
        add_executable(TexturePacker::TexturePacker::Executable IMPORTED GLOBAL)
        set_target_properties(TexturePacker::TexturePacker::Executable PROPERTIES
                                          IMPORTED_LOCATION "${TEXTUREPACKER_EXECUTABLE}")
        message(STATUS "Found external TexturePacker: ${TEXTUREPACKER_EXECUTABLE}")
      else()
        # Warn about external TexturePacker supplied but not fail fatally
        # because we might have internal TexturePacker executable built
        # and unset TEXTUREPACKER_EXECUTABLE variable
        message(WARNING "Could not find '${APP_NAME_LC}-TexturePacker' or 'TexturePacker' executable in ${_tppath} supplied by -DWITH_TEXTUREPACKER. Make sure the executable file name matches these names!")
      endif()
    endif()

    # Ship TexturePacker only on Linux and FreeBSD
    if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
      # But skip shipping it if build architecture can be executed on host
      # and TEXTUREPACKER_EXECUTABLE is found
      if(NOT (HOST_CAN_EXECUTE_TARGET AND TEXTUREPACKER_EXECUTABLE))
        set(INTERNAL_TEXTUREPACKER_INSTALLABLE TRUE CACHE BOOL "" FORCE)
      endif()
    endif()

    # Use it during build if build architecture can be executed on host
    # and TEXTUREPACKER_EXECUTABLE is not found
    if(HOST_CAN_EXECUTE_TARGET AND NOT TEXTUREPACKER_EXECUTABLE)
      set(INTERNAL_TEXTUREPACKER_EXECUTABLE TRUE)
    endif()

    # Build and install internal TexturePacker if needed
    if (INTERNAL_TEXTUREPACKER_EXECUTABLE OR INTERNAL_TEXTUREPACKER_INSTALLABLE)
      set(CMAKE_ARGS "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS} $<$<CONFIG:Debug>:${CMAKE_C_FLAGS_DEBUG}> $<$<CONFIG:Release>:${CMAKE_C_FLAGS_RELEASE}>"
                     "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} $<$<CONFIG:Debug>:${CMAKE_CXX_FLAGS_DEBUG}> $<$<CONFIG:Release>:${CMAKE_CXX_FLAGS_RELEASE}>"
                     "-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS} $<$<CONFIG:Debug>:${CMAKE_EXE_LINKER_FLAGS_DEBUG}> $<$<CONFIG:Release>:${CMAKE_EXE_LINKER_FLAGS_RELEASE}>"
                     "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
                     "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
                     "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
                     "-DCMAKE_AR=${CMAKE_AR}"
                     "-DCMAKE_LINKER=${CMAKE_LINKER}"
                     "-DCMAKE_NM=${CMAKE_NM}"
                     "-DCMAKE_STRIP=${CMAKE_STRIP}"
                     "-DCMAKE_OBJDUMP=${CMAKE_OBJDUMP}"
                     "-DCMAKE_RANLIB=${CMAKE_RANLIB}"
                     -DKODI_SOURCE_DIR=${CMAKE_SOURCE_DIR}
                     -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/build)

      # Create a list with an alternate separator e.g. pipe symbol
      string(REPLACE ";" "|" string_ARCH_DEFINES "${ARCH_DEFINES}")

      list(APPEND CMAKE_ARGS -DARCH_DEFINES=${string_ARCH_DEFINES})

      externalproject_add(buildtexturepacker
                          SOURCE_DIR ${CMAKE_SOURCE_DIR}/tools/depends/native/TexturePacker/src
                          PREFIX ${CORE_BUILD_DIR}/build-texturepacker
                          LIST_SEPARATOR |
                          INSTALL_DIR ${CMAKE_BINARY_DIR}/build
                          CMAKE_ARGS ${CMAKE_ARGS}
                          BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/build/bin/TexturePacker)

      ExternalProject_Get_Property(buildtexturepacker INSTALL_DIR)
      add_executable(TexturePacker IMPORTED)
      set_target_properties(TexturePacker PROPERTIES
                                          IMPORTED_LOCATION "${INSTALL_DIR}/bin/TexturePacker")

      add_dependencies(TexturePacker buildtexturepacker)

      message(STATUS "Building internal TexturePacker")
    endif()

    if(INTERNAL_TEXTUREPACKER_INSTALLABLE)
      add_executable(TexturePacker::TexturePacker::Installable ALIAS TexturePacker)
      message(STATUS "Shipping internal TexturePacker")
    endif()

    if(INTERNAL_TEXTUREPACKER_EXECUTABLE)
      add_executable(TexturePacker::TexturePacker::Executable ALIAS TexturePacker)
      message(STATUS "Internal TexturePacker will be executed during build")
    else()
      message(STATUS "External TexturePacker will be executed during build: ${TEXTUREPACKER_EXECUTABLE}")

      include(FindPackageHandleStandardArgs)
      find_package_handle_standard_args(TexturePacker DEFAULT_MSG TEXTUREPACKER_EXECUTABLE)
    endif()

    mark_as_advanced(INTERNAL_TEXTUREPACKER_EXECUTABLE INTERNAL_TEXTUREPACKER_INSTALLABLE TEXTUREPACKER)
  endif()
endif()
