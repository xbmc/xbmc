if(ENABLE_MOLD)
  if(CMAKE_CXX_COMPILER_ID STREQUAL GNU AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 12)
    # GCC < 12 doesn't support -fuse-ld=mold, so we have to use tools prefix path
    # if mold is installed in a non-standard dir, users can set -DMOLD_PREFIX=/path/to/mold_install_prefix
    find_path(MOLD_PREFIX_DIR NAMES ld
                              NO_DEFAULT_PATH # this is needed, otherwise we find /usr/bin/ld
                              PATH_SUFFIXES libexec/mold
                              PATHS ${MOLD_PREFIX} /usr /usr/local)
    if(MOLD_PREFIX_DIR)
      set(COMPILER_ARGS "-B${MOLD_PREFIX_DIR}")
    else()
      message(WARNING "Mold LD path not found, you might need to set -DMOLD_PREFIX=/path/to/mold_install_prefix")
    endif()
  else()
    set(COMPILER_ARGS "-fuse-ld=mold")
  endif()

  execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${COMPILER_ARGS} -Wl,--version ERROR_QUIET OUTPUT_VARIABLE LD_VERSION)

  set(DEFAULT_ENABLE_DEBUGFISSION FALSE)
  if(CMAKE_BUILD_TYPE STREQUAL Debug OR
     CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
    set(DEFAULT_ENABLE_DEBUGFISSION TRUE)
  endif()

  include(CMakeDependentOption)
  cmake_dependent_option(ENABLE_DEBUGFISSION "Enable Debug Fission support" ON
                         "DEFAULT_ENABLE_DEBUGFISSION" OFF)

  if(ENABLE_DEBUGFISSION)
    include(TestCXXAcceptsFlag)
    check_cxx_accepts_flag(-gsplit-dwarf CXX_ACCEPTS_GSPLIT_DWARF)

    # extract mold version
    set(LD_VERSION_LIST ${LD_VERSION})
    separate_arguments(LD_VERSION_LIST)
    list(GET LD_VERSION_LIST 1 MOLD_VERSION)

    set(DEBUGFISSION_AVAILABLE FALSE)
    if(CXX_ACCEPTS_GSPLIT_DWARF)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gsplit-dwarf")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gsplit-dwarf")
      if(${MOLD_VERSION} VERSION_GREATER_EQUAL "1.2.0")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gdb-index")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gdb-index")
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gdb-index")
        set(DEBUGFISSION_AVAILABLE TRUE)
      endif()
    endif()

    if(DEBUGFISSION_AVAILABLE)
      message(STATUS "Debug Fission enabled")
    else()
      message(WARNING "Debug Fission is not available")
    endif()
  endif()

  if(LD_VERSION MATCHES "mold")
    include(FindPackageHandleStandardArgs)
    find_program(MOLD_EXECUTABLE mold)
    find_package_handle_standard_args(MOLD REQUIRED_VARS MOLD_EXECUTABLE)

    if(MOLD_FOUND)
      set(CMAKE_LINKER ${MOLD_EXECUTABLE})
      set(CMAKE_CXX_LINK_FLAGS ${COMPILER_ARGS})
      set(CMAKE_C_LINK_FLAGS ${COMPILER_ARGS})
      set(CMAKE_EXE_LINKER_FLAGS "${LD_FLAGS} ${COMPILER_ARGS} ${CMAKE_EXE_LINKER_FLAGS}")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
      set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
      message(STATUS "Linker: mold")
    endif()
    mark_as_advanced(MOLD_EXECUTABLE CMAKE_LINKER)

  else()
    message(FATAL_ERROR "Mold linker not found")
  endif()
endif()
