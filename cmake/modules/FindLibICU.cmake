# .rst : FindLibICU
# ------------
# Finds, downloads and builds the ICU libraries, as necessary
#
# This module will first look for the required library versions on the system.
# If they are not found, it will fall back to downloading and building kodi's
# own version
#
# --------
# The following variables influence behaviour:
# ENABLE_INTERNAL_LIBICU - if enabled, any local LIBICU will be ignored. 
#               LIBICU source will be downloaded and built instead.
#
# --------
# This module will define the following variables:
#
# LIBICU_FOUND - system has LIBICU 
# LIBICU_INCLUDE_DIRS - the LIBICU include directory
# LIBICU_LIBRARIES - the LIBICU libraries
# LIBICU_DEFINITIONS - the LIBICU build definitions (-Ddefine)
#
# and the following imported targets:
#
# libicu   - The LibICU library
#
# Author: Frank Feuerbacher
# Based on FindFFMPEG.cmake
# ONLY tested on Linux (Ubuntu 22.04):
# - Tested using ENABLE_INTERNAL_LIBICU ON and OFF
#

set(VERBOSE ON)
include(CMakePrintHelpers)

# Constants/default values. Version and other info are also in
# LIBICU-VERSION

set(MODULE_LC libicu)
set(MIN_LIBICU_VERSION 67.0.0)
set(MAX_LIBICU_VERSION 71.1.0)
set(MODULE_LC libicu)
set(DEBUG_ICU ON)
set(LIBICU_LIB_TYPE SHARED) # Only SHARED tested

# set(LIBICU_URL
# "https://github.com/unicode-org/icu/releases/download/release-71-1/icu4c-71_1-src.tgz")

function(dump_cmake_variables)
  get_cmake_property(_variableNames VARIABLES)
  list(SORT _variableNames)
  foreach(_variableName ${_variableNames})
    if(ARGV0)
      unset(MATCHED)
      string(REGEX MATCH ${ARGV0} MATCHED ${_variableName})
      if(NOT MATCHED)
        continue()
      endif()
    endif()
    message(STATUS "${_variableName}=${${_variableName}}")
  endforeach()
endfunction()

set(LIBICU_FOUND 0)

macro(configure)
  set(LIBICU_URL_PROVIDED TRUE)

  if(U_DISABLE_RENAMING)
    message("Disabling renaming of ICU symbols due to -DU_DISABLE_RENAMING set")
    set(ENABLE_RENAMING OFF)
  else()
    set(ENABLE_RENAMING ON)
  endif()

  if(NOT LIBICU_LIB_TYPE)
    set(LIBICU_LIB_TYPE SHARED)
  endif()

  # Check for existing LibICU. Suppress mismatch warning, see
  # https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
  # set(FPHSA_NAME_MISMATCHED 1) find_package(LibDvdRead MODULE REQUIRED)
  # unset(FPHSA_NAME_MISMATCHED)

  # We require this due to the odd nature of github URL's compared to our other
  # tarball mirror system. If User sets LIBICU_URL or libicu_URL, allow
  # get_filename_component in SETUP_BUILD_VARS

  if(LIBICU_URL OR ${MODULE_LC}_URL)
    if(${MODULE_LC}_URL)
      set(LIBICU_URL ${${MODULE_LC}_URL})
    endif()
    set(LIBICU_URL_PROVIDED TRUE)
  endif()

  # Must be done AFTER setting LIBICU_URL. Reads VERSION file, etc.

  setup_build_vars()

  set(LIBICU_VERSION ${MAX_LIBICU_VERSION})
  set(LIBICU_CONFIG_PLATFORM Linux) # Need a better solution later
  set(LIBICU_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)
  set(LIBICU_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
  set(LIBICU_DEFINITIONS -DUSE_STATIC_LIBICU=0)
  message("MODULE_VER: ${${MODULE}_VER}")

  set(BUILD_ENV_VARS
      # ICU_VER=${LIBICU_VER}
      # U_DISABLE_RENAMING=0  # Must match ConfigArgs setting. Useful for
      # debugging. If set to 1, must specify it to top level CMakeLists as
      # -DU_DISBLE_RENAMING=1
      U_DEFAULT_SHOW_DRAFT=0
      U_HIDE_DRAFT_API=1
      U_HIDE_DEPRECATED_API=1
      U_HIDE_OBSOLETE_UTF_OLD_H=1
      U_NO_DEFAULT_INCLUDE_UTF_HEADERS=1
      UNISTR_FROM_CHAR_EXPLICIT=explicit
      UNISTR_FROM_STRING_EXPLICIT=explicit
      UCONFIG_NO_LEGACY_CONVERSION=1
      UNISTR_OBJECT_SIZE=256)

  message("BUILD_ENV_VARS: ${BUILD_ENV_VARS}")
  
  set(ConfigArgs
      --disable-draft
      --disable-extras
      --enable-icu-config # A command
      --disable-layoutex # Default
      --disable-samples
      --enable-strict
      --disable-tests
      --enable-tools
      --with-data-packaging=library)
  message("ConfigArgs: ${ConfigArgs}")

 if(ANDROID)
    set(library_suffix .so)
    list(APPEND ConfigArgs --with-library-suffix="${library_suffix}")
    #list(APPEND CMAKE_C_FLAGS
    #     -fno-short-wchar
    #     -fno-short-enums
    #     -ffunction-sections
    #     -fdata-sections 
    #     -fvisibility=hidden)
    message("ANDROID ConfigArgs: ${ConfigArgs}")   
  endif()
  
  if(ENABLE_INTERNAL_LIBICU) # Build our own instead of using system libicu
    set(SYSTEM_ICU OFF)
  else()
    set(SYSTEM_ICU ON)
  endif()

  if(DEBUG_ICU)
    list(APPEND ConfigArgs --enable-debug --disable-release)
  else()
    list(APPEND ConfigArgs --disable-debug --enable-release)
  endif()

  message("ConfigArgs: ${ConfigArgs}")
  
  # By default, every function call is renamed with a version suffix to
  # guarantee that you link with same version you compiled with. Complicates
  # finding symbols with IDE, etc. Note that this setting impacts both the
  # libraries that at created (--enable-renaming) as well as the functions
  # called by application code (-DU_DISABLE_RENAMING)

  # list(APPEND icu_config_env ICU_VER=${LIBICU_VER})

  if(ENABLE_RENAMING)
    list(APPEND ConfigArgs --enable-renaming)
    list(APPEND BUILD_ENV_VARS U_DISABLE_RENAMING=0)
    add_compile_definitions(INTERFACE PUBLIC U_DISABLE_RENAMING=0)
  else()
    list(APPEND ConfigArgs --disable-renaming)
    list(APPEND BUILD_ENV_VARS U_DISABLE_RENAMING=1)

    # Must define this while compiling anything that includes libicu

    add_compile_definitions(INTERFACE PUBLIC U_DISABLE_RENAMING=1)
  endif()

  message("ConfigArgs: ${ConfigArgs}")
  message("BUILD_ENV_VARS: ${BUILD_ENV_VARS}")
  
  if(LIBICU_LIB_TYPE STREQUAL SHARED)
    list(APPEND ConfigArgs --enable-shared)
    list(APPEND BUILD_ENV_VARS ICU_STATIC=NO)
  else()
    list(APPEND ConfigArgs --disable-shared)
    list(APPEND BUILD_ENV_VARS ICU_STATIC=YES)
  endif()

  message("ConfigArgs: ${ConfigArgs}")
  message("BUILD_ENV_VARS: ${BUILD_ENV_VARS}")
  message("CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")
  message("CMAKE_LINKER_FLAGS: ${CMAKE_LINKER_FLAGS}")
  message("CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
  message("SYSTEM_LDFLAGS: ${SYSTEM_LDFLAGS}")

  # Somewhere along the line, there is a leading space that must be removed.

  string(STRIP ${CMAKE_SHARED_LINKER_FLAGS} LIBICU_SHARED_LINKER_FLAGS)
  if(VERBOSE)
    cmake_print_variables(ConfigArgs)
    cmake_print_variables(BUILD_ENV_VARS)
  endif()

  set(MAKE_COMMAND $(MAKE))
  if(CMAKE_GENERATOR STREQUAL Ninja)
    set(MAKE_COMMAND make)
    include(ProcessorCount)
    ProcessorCount(N)
    if(NOT N EQUAL 0)
      set(MAKE_COMMAND make -j${N})
    endif()
  endif()

  # if(DEBUG_ICU) set(LIBICU_BUILD_TYPE "Debug") else() set(LIBICU_BUILD_TYPE
  # "Release") endif()

  set(LIBICU_OPTIONS
      -DENABLE_CCACHE=${ENABLE_CCACHE} -DCCACHE_PROGRAM=${CCACHE_PROGRAM}
      -DEXTRA_FLAGS=${LIBICU_EXTRA_FLAGS} -DLIBICU_LIB_TYPE=${LIBICU_LIB_TYPE})
  message("FindLIBICU KODI_DEPENDSBUILD: ${KODI_DEPENDSBUILD}")
  if(KODI_DEPENDSBUILD)
    message("KODI_DEPENDSBUILD")
    set(CROSS_ARGS
        -DDEPENDS_PATH=${DEPENDS_PATH}
        -DPKG_CONFIG_EXECUTABLE=${PKG_CONFIG_EXECUTABLE}
        -DCROSSCOMPILING=${CMAKE_CROSSCOMPILING}
        -DOS=${OS}
        -DCMAKE_AR=${CMAKE_AR})
    message("CROSS_ARGS: ${CROSS_ARGS}")
  endif()

  if(VERBOSE)
    message("libicu start dump_cmake_variables")
    dump_cmake_variables()
    message("libicu end dump_cmake_variables")

    message("LIB_TYPE: ${LIB_TYPE}")

    message(
      STATUS
        "\n#--------------- libicu CMakeLists.txt Internal Variables -------------#"
    )

    message(
      "CONFIGURE_COMMAND: ${CONFIGURE_COMMAND}<SOURCE_DIR>/runConfigureICU")
    message("LIBICU_CONFIG_PLATFORM: ${LIBICU_CONFIG_PLATFORM}")
    message("LIBICU_INSTALL_PREFIX: ${LIBICU_INSTALL_PREFIX}")
    message("BUILD_COMMAND: ${BUILD_COMMAND} ${MAKE_COMMAND}")
    message("LIBICU_BUILD_TYPE: ${LIBICU_BUILD_TYPE}")
    message("BUILD_TYPE: ${BUILD_TYPE}")

    message("CCACHE_PROGRAM: ${CCACHE_PROGRAM}")
    message("LIBICU_LIB_TYPE: ${LIBICU_LIB_TYPE}")
    message("SOURCE_DIR: ${SOURCE_DIR}")

    message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
    message("Prefix: ${Prefix}")
    message("DOWNLOAD_DIR: ${DOWNLOAD_DIR}")
    message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
    message("CORE_BUILD_DIR: ${CORE_BUILD_DIR}")
    message("CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
    message("CORE_SYSTEM_NAME: ${CORE_SYSTEM_NAME}")
    message("CORE_PLATFORM_NAME: ${CORE_PLATFORM_NAME}")
    message("CPU: ${CPU}")
    message("CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
    message("CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
    message("ENABLE_CCACHE: ${ENABLE_CCACHE}")
    message("CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
    message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
    message("LIBICU_SHARED_LINKER_FLAGS: ${LIBICU_SHARED_LINKER_FLAGS}")
    message("CROSS_ARGS: ${CROSS_ARGS}")

    message("LIBICU_OPTIONS: ${LIBICU_OPTIONS}")
    message(
      "PKG_CONFIG_PATH will be: ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/pkgconfig" # /scratch/frank/Source/v20submit.build/kodi-build-master/build/lib/pkgconfig
    )
    message("PKG_CONFIG_FOUND: ${PKG_CONFIG_FOUND}") # FALSE
    message("PKG_CONFIG_EXECUTABLE: ${PKG_CONFIG_EXECUTABLE}") #
    message("PATCH_COMMAND: ${PATCH_COMMAND}")
    message("CMAKE_GENERATOR: ${CMAKE_GENERATOR}")
    message("LIBICU_INCLUDE_DIRS: ${LIBICU_INCLUDE_DIRS}")
    message("LIBICU_INCLUDE_DIR: ${LIBICU_INCLUDE_DIR}")
  endif()

  list(APPEND BUILD_ENVIRON_VARS CFLAGS=${CMAKE_C_FLAGS}
       CXX_FLAGS=${CMAKE_CXX_FLAGS} PKG_CONFIG=${PKG_CONFIG_EXECUTABLE}
       LDFLAGS=${LIBICU_SHARED_LINKER_FLAGS})
  # list(APPEND BUILD_ENV_VARS ${icu_config_env})

  if(APPLE)
    set(LIBICU_LDFLAGS "-framework CoreFoundation")
    string(APPEND LIBICU_CFLAGS " -D__DARWIN__")
    if(NOT CORE_SYSTEM_NAME STREQUAL darwin_embedded)
      string(APPEND LIBICU_LDFLAGS " -framework IOKit")
    endif()
  endif()

  if(CORE_SYSTEM_NAME MATCHES windows)
    set(CMAKE_ARGS -DDUMMY_DEFINE=ON ${LIBDVD_ADDITIONAL_ARGS})
  else()
    find_program(MAKE_EXECUTABLE make REQUIRED)

    if(CMAKE_GENERATOR STREQUAL Xcode)
      set(LIBICU_GENERATOR CMAKE_GENERATOR "Unix Makefiles")
    endif()

    # BUILD_DEP_TARGET will use this to: * Download source * Configure source
    # via .../libicu/CMakeLists.txt * Build & install LibiICU
    #
    set(CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        -DCMAKE_MODULE_PATH=${LIBICU_MODULE_PATH}
        # -DICU_VER=${MAX_LIBICU_VERSION}
        -DCORE_SYSTEM_NAME=${CORE_SYSTEM_NAME}
        -DCORE_PLATFORM_NAME=${CORE_PLATFORM_NAME_LC}
        -DCPU=${CPU}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DENABLE_CCACHE=${ENABLE_CCACHE}
        -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        -DCMAKE_EXE_LINKER_FLAGS=${LIBICU_SHARED_LINKER_FLAGS}
        -DLIBICU_BUILD_TYPE=${LIBICU_BUILD_TYPE}
        ${CROSS_ARGS}
        ${LIBICU_OPTIONS}
        -DPKG_CONFIG_PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/pkgconfig)

    set(CONFIGURE_COMMAND
        # SOURCE_DIR ${CMAKE_SOURCE_DIR}/source
        COMMAND ${CMAKE_COMMAND} -E env ${BUILD_ENVIRON_VARS}
        <SOURCE_DIR>/source/runConfigureICU ${LIBICU_CONFIG_PLATFORM}
        --prefix=${DEPENDS_PATH} --libdir=${LIBICU_LIBDIR}
        --includedir=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include
        ${ConfigArgs} BUILD_COMMAND) # Runs make to build after the download and
                                     # config.

    cmake_print_variables(CONFIGURE_COMMAND)
   
    cmake_print_variables(CMAKE_ARGS)
    
    # set(BUILD_COMMAND "/usr/bin/bash ls -l
    # ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include")

    set(BUILD_COMMAND ${MAKE_EXECUTABLE})
    set(INSTALL_COMMAND ${MAKE_EXECUTABLE} install)
    set(BUILD_IN_SOURCE 0)
  endif()
endmacro()

macro(buildLIBICU)
  include(cmake/scripts/common/ModuleHelpers.cmake)
  message("In macro buildLIBICU")

  configure()

  # Macro to create externalproject_add target
  #
  # Common usage
  #
  # CMAKE_ARGS: cmake(required) PATCH_COMMAND: ALL(optional) CONFIGURE_COMMAND:
  # autoconf(required), meson(required) BUILD_COMMAND: autoconf(required),
  # meson(required), cmake(optional) INSTALL_COMMAND: autoconf(required),
  # meson(required), cmake(optional) BUILD_IN_SOURCE: ALL(optional)
  # BUILD_BYPRODUCTS: ALL(optional)

  build_dep_target()
endmacro()

if(NOT TARGET LibICU::LibICU AND ENABLE_INTERNAL_LIBICU)
  message("Calling buildLIBICU")
  buildlibicu()
  message("Returned from buildLIBICU")
else()
  # Try to use locally installed libICU
  #
  # LIBICU_INCLUDE_DIRS - the LIBICU include directory LIBICU_LIBRARIES - the
  # LIBICU libraries LIBICU_DEFINITIONS - the  LIBICU build
  # definitions(-Ddefine)
  #
  if(PKG_CONFIG_FOUND)
    # All should be at same version and all should be present(not sure if more
    # than one needs to be checked).
    #
    unset(LIBICU_INCLUDE_DIRS)
    #
    # Prefer MAX_LIBICU_VERSION, but take anything within range
    #
    set(module "PC_ICU_I18N_LIBRARY")
    pkg_search_module(
      ${module} IMPORTED_TARGET icu-i18n=${MAX_LIBICU_VERSION}
      icu-i18n>=${MIN_LIBICU_VERSION} icu-i18n<=${MAX_LIBICU_VERSION})

    dump_cmake_variables("${module}.*")
    message("dump_cmake_variables finished")
    pkg_get_variable(x icu-i18n FOUND)
    message("${module}_FOUND: ${${module}_FOUND}")
    message("${module}_LINK_LIBRARIES: ${${module}_LINK_LIBRARIES}")
    message("${module}_LIBRARIES: ${${module}_LIBRARIES}")
    # message("${module}_LIBRARY_DIRS: ${${module}_LIBRARY_DIRS}")
    message("${module}_LDFLAGS: ${${module}_LDFLAGS}")
    # message("${module}_INCLUDE_DIRS: ${${module}_INCLUDE_DIRS}")
    message("${module}_INCLUDEDIR: ${${module}_INCLUDEDIR}")
    # message("${module}_LDFLAGS_OTHER: ${${module}_LDFLAGS_OTHER}")
    # message("${module}_CFLAGS: ${${module}_CFLAGS}")
    # message("${module}_CFLAGS_OTHER: ${${module}_CFLAGS_OTHER}")
    message(
      "${module}_INTERFACE_COMPILE_OPTIONS: ${${module}_INTERFACE_COMPILE_OPTIONS}"
    )
    # message("${module}_INTERFACE_INCLUDE_DIRECTORIES:
    # ${${module}_INTERFACE_INCLUDE_DIRECTORIES}")
    message("${module}_LIBDIR: ${${module}_LIBDIR}")
    message("${module}_MODULE_NAME: ${${module}_MODULE_NAME}")
    message("${module}_VERSION: ${${module}_VERSION}")

    set(LOCAL_LIBICU_VERSION ${${module}_VERSION})
    message("LOCAL_LIBICU_VERSION: ${LOCAL_LIBICU_VERSION}")
    if(LOCAL_LIBICU_VERSION)
      set(LIBICU_FOUND 1)
    endif()

    foreach(module_suffix i18n io uc)
      set(module icu-${module_suffix})
      message("module: ${module}")
      string(TOUPPER PC_ICU_${module_suffix} prefix)
      message("prefix: ${prefix}")
      message("LOCAL_LIBICU_VERSION: ${LOCAL_LIBICU_VERSION}")
      if(LOCAL_LIBICU_VERSION)
        pkg_check_modules(${prefix} ${module}=${LOCAL_LIBICU_VERSION})
        message("prefix_VERSION: ${${prefix}_VERSION}")
        if(${prefix}_VERSION)
          message(
            "LOCAL_LIBICU_VERSION: ${LOCAL_LIBICU_VERSION} prefix: ${${prefix}_VERSION}"
          )
          if(NOT (LOCAL_LIBICU_VERSION STREQUAL ${prefix}_VERSION))
            message(
              "Did not find ${module} library with matching version to icu-i18n lib"
            )
            set(LIBICU_FOUND 0)
          else()
            message("appending ${${prefix}_LINK_LIBRARIES}")
            list(APPEND LIBICU_LIBS "${${prefix}_LINK_LIBRARIES}")
            message("prefix_INCLUDEDIR: ${${prefix}_INCLUDEDIR}")
            if(NOT LIBICU_INCLUDE_DIRS AND ${prefix}_INCLUDEDIR)
              message("setting LIBICU_INCLUDE_DIRS")
              set(LIBICU_INCLUDE_DIRS ${${prefix}_INCLUDEDIR})
            endif()
            # Assume that every library entry has same LIBDIR
            if(${prefix}_LIBDIR)
              set(LIBICU_LIBDIR ${${prefix}_LIBDIR})
            endif()
          endif()
        endif()
      endif()
    endforeach()

    message("LIBICU_INCLUDE_DIRS: ${LIBICU_INCLUDE_DIRS}")
    list(REMOVE_DUPLICATES LIBICU_LIBS)
    message("LIBICU_LIBS: ${LIBICU_LIBS}")
  endif()
  if(VERBOSE)
    message(
      "LIBICU_FOUND: ${LIBICU_FOUND} LIBICU_VERSION: ${LOCAL_LIBICU_VERSION}")
  endif()
  if(LIBICU_FOUND)
    set(LIBICU_LIBRARIES ${LIBICU_LIBS})
    message(
      "LIBICU_LIBRARIES: ${LIBICU_LIBRARIES} LIBICU_INCLUDE_DIRS: ${LIBICU_INCLUDE_DIRS}"
    )
  else()
    message(
      STATUS
        "LibICU ${MAX_LIBICU_VERSION} not found, falling back to internal build"
    )
    buildlibicu()
  endif()
endif()

if(LIBICU_FOUND)
  if(NOT TARGET libicu)
    add_custom_target(libicu)
    message("LIBICU_INCLUDE_DIRS: ${LIBICU_INCLUDE_DIRS}")
    message("LIBICU_LIBDIR: ${LIBICU_LIBDIR}")
    set(ICU_I18N_LIBRARY ${LIBICU_LIBDIR}/libicui18n.so)
    set(ICU_IO_LIBRARY ${LIBICU_LIBDIR}/libicuio.so)
    set(ICU_UC_LIBRARY ${LIBICU_LIBDIR}/libicuuc.so)
    set(ICU_DATA_LIBRARY ${LIBICU_LIBDIR}/libicudata.so)

    set(LIBICU_LIBRARIES ${ICU_I18N_LIBRARY} ${ICU_IO_LIBRARY}
                         ${ICU_UC_LIBRARY} ${ICU_DATA_LIBRARY})
  endif()
  add_library(LibICU::LibICU_I18N ${LIBICU_LIB_TYPE} IMPORTED)
  add_library(LibICU::LibICU_DATA ${LIBICU_LIB_TYPE} IMPORTED)
  add_library(LibICU::LibICU_IO ${LIBICU_LIB_TYPE} IMPORTED)
  add_library(LibICU::LibICU_UC ${LIBICU_LIB_TYPE} IMPORTED)

  set_target_properties(
    LibICU::LibICU_I18N
    PROPERTIES FOLDER "External Projects"
               IMPORTED_LOCATION "${ICU_I18N_LIBRARY}"
               IMPORTED_SONAME ON
               INTERFACE_INCLUDE_DIRECTORIES "${LIBICU_INCLUDE_DIRS}"
               INTERFACE_LINK_LIBRARIES "${ICU_I18N_LIBRARY}")
  set_target_properties(
    LibICU::LibICU_DATA
    PROPERTIES FOLDER "External Projects"
               IMPORTED_LOCATION "${ICU_DATA_LIBRARY}"
               IMPORTED_SONAME ON
               INTERFACE_INCLUDE_DIRECTORIES "${LIBICU_INCLUDE_DIRS}"
               INTERFACE_LINK_LIBRARIES "${ICU_DATA_LIBRARY}")

  set_target_properties(
    LibICU::LibICU_IO
    PROPERTIES FOLDER "External Projects"
               IMPORTED_LOCATION "${ICU_IO_LIBRARY}"
               IMPORTED_SONAME ON
               INTERFACE_INCLUDE_DIRECTORIES "${LIBICU_INCLUDE_DIRS}"
               INTERFACE_LINK_LIBRARIES "${ICU_IO_LIBRARY}")
  set_target_properties(
    LibICU::LibICU_UC
    PROPERTIES FOLDER "External Projects"
               IMPORTED_LOCATION "${ICU_UC_LIBRARY}"
               IMPORTED_SONAME ON
               INTERFACE_INCLUDE_DIRECTORIES "${LIBICU_INCLUDE_DIRS}"
               INTERFACE_LINK_LIBRARIES "${ICU_UC_LIBRARY}")

  add_dependencies(LibICU::LibICU_I18N libicu)
  add_dependencies(LibICU::LibICU_IO libicu)
  add_dependencies(LibICU::LibICU_UC libicu)
  add_dependencies(LibICU::LibICU_DATA libicu)

  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP LibICU::LibICU_I18N)
  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP LibICU::LibICU_IO)
  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP LibICU::LibICU_UC)
  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP LibICU::LibICU_DATA)
else()
  if(LIBICU_FIND_REQUIRED)
    message(FATAL_ERROR "LibICU not found")
  endif()
endif()

# Not sure how much of this applies

if(APPLE)
  string(APPEND LIBICU_DEFINITIONS " -D__DARWIN__")
endif()

mark_as_advanced(LIBICU_INCLUDE_DIRS ICU_I18N_LIBRARY ICU_IO_LIBRARY
                 ICU_UC_LIBRARY ICU_DATA_LIBRARY)
