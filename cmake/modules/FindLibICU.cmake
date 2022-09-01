# .rst: FindICU
# ------------
# Finds, downloads and builds the ICU libraries, as necessary
#
# This module will first look for the required library versions on the system.
# If they are not found, it will fall back to downloading and building kodi's own version
#
# --------
# The following variables influence behaviour:
# ENABLE_INTERNAL_LIBICU - if enabled, any local LIBICU will be ignored. 
#               LIBICU source will be downloaded and built instead.
#
# This will define the following variables::
#
# LIBICU_FOUND - system has LIBICU 
# LIBICU_INCLUDE_DIRS - the LIBICU include directory
# LIBICU_LIBRARIES - the LIBICU libraries
# LIBICU_DEFINITIONS - the LIBICU build definitions (-Ddefine)
#
# and the following imported targets::
#
# libicu   - The LibICU library
#
# Author: Frank Feuerbacher
# Based on FindFFMPEG.cmake
# ONLY tested on Linux (Ubuntu 22.04):
# - Tested using ENABLE_INTERNAL_LIBICU ON and OFF
#


# set(VERBOSE ON)
include(CMakePrintHelpers)

# Constants/default values. Version and other info are also in
# LIBICU-VERSION

set(MIN_LIBICU_VERSION 67.0.0)
set(MAX_LIBICU_VERSION 71.1.0)
   set(DEFAULT_LIBICU_URL
        "https://github.com/unicode-org/icu/releases/download/release-71-1/icu4c-71_1-src.tgz"
    )

set(ICU_LIB_TYPE SHARED)  # Only SHARED tested
set(LIBICU_FOUND 0)

function(dump_cmake_variables)
    get_cmake_property(_variableNames VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        if (ARGV0)
            unset(MATCHED)
            string(REGEX MATCH ${ARGV0} MATCHED ${_variableName})
            if (NOT MATCHED)
                continue()
            endif()
        endif()
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()


macro(buildLIBICU)
  include(cmake/scripts/common/ModuleHelpers.cmake)
  message("In macro buildLIBICU")

  set(${MODULE}_SOURCE_DIR ${CMAKE_SOURCE_DIR}/tools/depends/target/libicu)
  set(MODULE_LC libicu)

  # We require this due to the odd nature of github URL's compared to our other
  # tarball mirror system. If User sets LIBICU_URL, allow get_filename_component
  # in SETUP_BUILD_VARS
  
  message(LIBICU_URL: "${LIBICU_URL} default: ${DEFAULT_LIBICU_URL}")
  if(LIBICU_URL)
    set(LIBICU_URL_PROVIDED TRUE)
  endif()
  set(PKG_CONFIG_FOUND FALSE)

  SETUP_BUILD_VARS()

  if(NOT LIBICU_URL_PROVIDED)
    # override LIBICU_URL due to tar naming when retrieved from github
    
    set(LIBICU_URL ${DEFAULT_LIBICU_URL})
    message("New LIBICU_URL: ${LIBICU_URL}")
  endif()

  set(LIBICU_OPTIONS
      -DENABLE_CCACHE=${ENABLE_CCACHE} -DCCACHE_PROGRAM=${CCACHE_PROGRAM}
      -DEXTRA_FLAGS=${LIBICU_EXTRA_FLAGS}
      -DENABLE_INTERNAL_LIBICU=${ENABLE_INTERNAL_LIBICU}
      -DICU_LIB_TYPE=${ICU_LIB_TYPE}
      )

#[[  Cross Compile copied from FindFFMPEG and NOT tested.

  if(KODI_DEPENDSBUILD)
    set(CROSS_ARGS
        -DDEPENDS_PATH=${DEPENDS_PATH}
        -DPKG_CONFIG_EXECUTABLE=${PKG_CONFIG_EXECUTABLE}
        -DCROSSCOMPILING=${CMAKE_CROSSCOMPILING}
        -DOS=${OS}
        -DCMAKE_AR=${CMAKE_AR})
  endif()

  message("CROSS_ARGS: ${CROSS_ARGS}")
]]
  set(LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
  list(APPEND LINKER_FLAGS ${SYSTEM_LDFLAGS})

  # Some list shenanigans not being passed through without stringify/listify
  # externalproject_add allows declaring list separator to generate a list for
  # the target
  string(REPLACE ";" "|" ICU_MODULE_PATH "${CMAKE_MODULE_PATH}")
  set(ICU_LIST_SEPARATOR LIST_SEPARATOR |)
  message("ICU_LIST_SEPARATOR: ${LIST_SEPARATOR}")

  if(VERBOSE)
    message(
      STATUS
        "\n#--------------- FindLibICU.cmake Internal Variables -------------#")

    message("SOURCE_DIR: ${SOURCE_DIR}")
    message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")   # Directory where top CMakeLists is (top of source tree)
    message("Prefix: ${Prefix}")
    message("Libdir: ${Libdir}")
    message("Bindir: ${Bindir}")
    message("Includedir: ${Includedir}")
    message("Datarootdir: ${Datarootdir}")
    message("Docdir: ${Docdir}")
    message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}") # ..../v20submit.build/kodi-build-master
    
    message("CORE_BUILD_DIR: ${CORE_BUILD_DIR}")  # build
    message("CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}") # /scratch/frank/Source/v20submit.build/master
    message("ICU_MODULE_PATH: ${ICU_MODULE_PATH}") # /scratch/frank/Source/v20submit/cmake/modules|/scratch/frank/Source/v20submit/cmake/modules/buildtools
    
    message("ICU_VER: ${ICU_VER}")
    message("CORE_SYSTEM_NAME: ${CORE_SYSTEM_NAME}") # linux
    message("CORE_PLATFORM_NAME: ${CORE_PLATFORM_NAME}") # x11
    message("CPU: ${CPU}")                               # x86-64
    message("ENABLE_NEON: ${ENABLE_NEON}")
    message("CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")   # /usr/bin/clang
    message("CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}") # /usr/bin/clang++
    message("ENABLE_CCACHE: ${ENABLE_CCACHE}")
    message("CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
    message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
    message("CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS} #######")  #  -fuse-ld=gold 
    message("CMAKE_EXE_LINKER_FLAGS: ${LINKER_FLAGS} #####")              #  -fuse-ld=gold 
    message("CROSS_ARGS: ${CROSS_ARGS}")
    message("LIBICU_OPTIONS: ${LIBICU_OPTIONS}")
    message(
      "PKG_CONFIG_PATH will be: ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/pkgconfig" # /scratch/frank/Source/v20submit.build/kodi-build-master/build/lib/pkgconfig
      
    )
    message("PKG_CONFIG_FOUND: ${PKG_CONFIG_FOUND}")           # FALSE
    message("PKG_CONFIG_EXECUTABLE: ${PKG_CONFIG_EXECUTABLE}")  # 
    message("PATCH_COMMAND: ${PATCH_COMMAND}")
    message("CMAKE_GENERATOR: ${CMAKE_GENERATOR}")
    message("LIBICU_INCLUDE_DIRS: ${LIBICU_INCLUDE_DIRS}")
    message("LIBICU_INCLUDE_DIR: ${LIBICU_INCLUDE_DIR}")
  endif()

  # BUILD_DEP_TARGET will use this to:
  #  - Download source
  #  - Configure source via .../libicu/CMakeLists.txt
  #  - Build & install LibiICU
  #
  set(CMAKE_ARGS
      -DENABLE_INTERNAL_LIBICU=${ENABLE_INTERNAL_LIBICU}
      -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
      -DCMAKE_MODULE_PATH=${ICU_MODULE_PATH}
      -DICU_VER=${MAX_LIBICU_VERSION}
      -DCORE_SYSTEM_NAME=${CORE_SYSTEM_NAME}
      -DCORE_PLATFORM_NAME=${CORE_PLATFORM_NAME_LC}
      -DCPU=${CPU}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DENABLE_CCACHE=${ENABLE_CCACHE}
      -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
      -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
      -DCMAKE_EXE_LINKER_FLAGS=${LINKER_FLAGS}
      ${CROSS_ARGS}
      ${LIBICU_OPTIONS}
      -DPKG_CONFIG_PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/pkgconfig)
  set(PATCH_COMMAND
      ${CMAKE_COMMAND} -E copy
      ${CMAKE_SOURCE_DIR}/tools/depends/target/libicu/CMakeLists.txt
      <SOURCE_DIR>)

  if(CMAKE_GENERATOR STREQUAL Xcode)
    set(ICU_GENERATOR CMAKE_GENERATOR "Unix Makefiles")
  endif()


  # Macro to create externalproject_add target
  #
  # Common usage
  #
  # CMAKE_ARGS: cmake(required) PATCH_COMMAND: ALL(optional) CONFIGURE_COMMAND:
  # autoconf(required), meson(required) BUILD_COMMAND: autoconf(required),
  # meson(required), cmake(optional) INSTALL_COMMAND: autoconf(required),
  # meson(required), cmake(optional) BUILD_IN_SOURCE: ALL(optional)
  # BUILD_BYPRODUCTS: ALL(optional)
  #
  if(VERBOSE)
  message("MODULE_LC: ${MODULE_LC}")
  message("MODULE: ${MODULE}")
  message("CMAKE_ARGS: ${CMAKE_ARGS}")
  message("PATCH_COMMAND: ${PATCH_COMMAND}")
  message("CONFIGURE_COMMAND: ${CONFIGURE_COMMAND}")
  message("BUILD_COMMAND: ${BUILD_COMMAND}")
  message("INSTALL_COMMAND: ${INSTALL-COMMAND}")
  message("BUILD_IN_SOURCE: ${BUILD_IN_SOURCE}")
  message("BUILD_BYPRODUCTS: ${BUILD_BYPRODUCTS}")
  endif()

  # Calls ExternalProject_Add
  BUILD_DEP_TARGET()
  
  set(LIBICU_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)
  set(LIBICU_DEFINITIONS -DUSE_STATIC_LIBICU=0)

  message("LIBICU_INCLUDE_DIRS: ${LIBICU_INCLUDE_DIRS}")

  set(LIBICU_LIBDIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib)

  set(ICU_I18N_LIBRARY ${LIBICU_LIBDIR}/libicui18n.so)
  set(ICU_IO_LIBRARY ${LIBICU_LIBDIR}/libicuio.so)
  set(ICU_UC_LIBRARY ${LIBICU_LIBDIR}/libicuuc.so)
  set(ICU_DATA_LIBRARY ${LIBICU_LIBDIR}/libicudata.so)

  set(LIBICU_LIBRARIES ${ICU_I18N_LIBRARY} ${ICU_IO_LIBRARY}
                       ${ICU_UC_LIBRARY} ${ICU_DATA_LIBRARY})
  set(LIBICU_INCLUDE_DIRS ${LIBICU_INCLUDE_DIR})

#  list(APPEND LIBICU_DEFINITIONS -DLIBICU_VER_SHA=\"${LIBICU_VERSION}\")

if(VERBOSE)
    message(
      "Adding target_properties to libicu:      FOLDER \"External Projects\"
                                 IMPORTED_LOCATION ${LIBICU_LIBRARIES}
                                 INTERFACE_INCLUDE_DIRECTORIES ${LIBICU_INCLUDE_DIRS}
                                 INTERFACE_LINK_LIBRARIES ${LIBICU_LDFLAGS}
                                 INTERFACE_COMPILE_DEFINITIONS ${LIBICU_DEFINITIONS}"
    )
endif()

    set_target_properties(
      libicu
      PROPERTIES FOLDER "External Projects"
                 IMPORTED_LOCATION "${LIBICU_LIBRARIES}"
                 INTERFACE_INCLUDE_DIRECTORIES "${LIBICU_INCLUDE_DIRS}"
                 INTERFACE_LINK_LIBRARIES "${LIBICU_LDFLAGS}"
                 INTERFACE_COMPILE_DEFINITIONS "${LIBICU_DEFINITIONS}")

  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP libicu)
  
endmacro()

message("ENABLE_INTERNAL_LIBICU: ${ENABLE_INTERNAL_LIBICU}")

if(ENABLE_INTERNAL_LIBICU)
  message("Calling buildLIBICU")
  buildLIBICU()
  message("Returned from buildLIBICU")
else()
  message("CONFIGURE_COMMAND: ${CONFIGURE_COMMAND}")
  
  if(PKG_CONFIG_FOUND)
    # All should be at same version and all should be present (not sure if 
    # more than one needs to be checked).
    #
    
    unset(LIBICU_INCLUDE_DIRS)
    
    # pkg_check_modules(PC_ICU_DATA_LIBRARY icu-data>=MIN_LIBICU_VERSION) # QUIET)
    pkg_search_module(PC_ICU_I18N_LIBRARY IMPORTED_TARGET
                                          icu-i18n=${MAX_LIBICU_VERSION}
                                          icu-i18n>=${MIN_LIBICU_VERSION} 
                                          icu-i18n<=${MAX_LIBICU_VERSION}  ) # QUIET)
    
    set(LOCAL_LIBICU_VERSION ${PC_ICU_I18N_LIBRARY_VERSION})
    set(LIBICU_LIBS "${PC_ICU_I18N_LIBRARY_LINK_LIBRARIES}")
    if(PC_ICU_I18N_LIBRARY_INCLUDE_DIRS)
       set(LIBICU_INCLUDE_DIRS ${PC_ICU_I18N_LIBRARY_INCLUDE_DIRS})
    endif()
    if(LOCAL_LIBICU_VERSION)
      pkg_check_modules(PC_ICU_IO_LIBRARY icu-io=${LOCAL_LIBICU_VERSION}) # QUIET)
      if(NOT ${PC_ICU_I18N_LIBRARY_icu-io_VERSION})
        unset(LOCAL_LIBICU_VERSION)
        message("Did not find icu-io library with matching version to i18n lib")
      else()
        list(APPEND LIBICU_LIBS "${PC_ICU_IO_LIBRARY_LINK_LIBRARIES}")        
        if(NOT LIBICU_INCLUDE_DIRS AND PC_ICU_IO_LIBRARY_INCLUDE_DIRS)
           set(LIBICU_INCLUDE_DIRS ${PC_ICU_IO_LIBRARY_INCLUDE_DIRS})
        endif()
      endif()
    endif()
    if(LOCAL_LIBICU_VERSION)
      pkg_check_modules(PC_ICU_UC_LIBRARY IMPORTED_TARGET icu-uc=${LOCAL_LIBICU_VERSION} ) # QUIET)
      if(NOT ${PC_ICU_I18N_LIBRARY_icu-uc_VERSION})
         message("Did not find icu-uc library with matching version to i18n lib")
         unset(LOCAL_LIBICU_VERSION)
      else()
         set(LIBICU_FOUND 1)
        list(APPEND LIBICU_LIBS "${PC_ICU_UC_LIBRARY_LINK_LIBRARIES}")        
        if(NOT LIBICU_INCLUDE_DIRS AND PC_ICU_UC_LIBRARY_INCLUDE_DIRS)
           set(LIBICU_INCLUDE_DIRS ${PC_ICU_UC_LIBRARY_INCLUDE_DIRS})
        endif()
      endif()
    endif()
    message("PC_ICU_UC_LIBRARY IMPORTED_TARGET:")
    cmake_print_variables(PkgConfig::PC_ICU_UC_LIBRARY)
    cmake_print_variables(LIBICU_LIBS)
    list(REMOVE_DUPLICATES LIBICU_LIBS)
  endif()
  if(VERBOSE)
  message("LIBICU_FOUND: ${LIBICU_FOUND} LIBICU_VERSION: ${LOCAL_LIBICU_VERSION}") 
  endif()  
  if(LIBICU_FOUND)
      set(LIBICU_LIBRARIES ${LIBICU_LIBS})
      set(LIBICU_INCLUDE_DIRS ${LIBICU_INCLUDE_DIR})
      message("LIBICU_LIBRARIES: ${LIBICU_LIBRARIES} LIBICU_INCLUDE_DIRS: ${LIBICU_INCLUDE_DIR}")
  else()
      message(
        STATUS
          "LibICU ${MAX_LIBICU_VERSION} not found, falling back to internal build"
      )
      buildLIBICU()
  endif()
endif()

# message("mark_as_advanced")

mark_as_advanced(ICU_INCLUDE_DIR ICU_I18N_LIBRARY ICU_IO_LIBRARY ICU_UC_LIBRARY
                 ICU_DATA_LIBRARY)
