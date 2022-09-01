project(
  libicu
  DESCRIPTION "Unicode Foundation ICU4C library"
  HOMEPAGE_URL "https://icu.unicode.org")

cmake_minimum_required(VERSION 3.12)
#
# Configures libicu libraries. Called by FindLibICU.
# Only tested for building shared libraries

include(CMakePrintHelpers)

set(VERBOSE ON)
  
set(DEBUG_ICU ON)
if(U_DISABLE_RENAMING)
  message("Disabling renaming of ICU symbols due to -DU_DISABLE_RENAMING set")
  set(ENABLE_RENAMING OFF)
else()
  set(ENABLE_RENAMING ON)
endif()

if(NOT ICU_LIB_TYPE)
  set(ICU_LIB_TYPE SHARED)
endif()

if(VERBOSE)
message(
  STATUS
    "\n#--------------- libicu CMakeLists.txt Internal Variables -------------#"
)

message("CCACHE_PROGRAM: ${CCACHE_PROGRAM}")
message("ICU_STATIC: ${ICU_STATIC}")
message("ENABLE_INTERNAL_LIBICU: ${ENABLE_INTERNAL_LIBICU}")
message("SOURCE_DIR: ${SOURCE_DIR}")

message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message("Prefix: ${Prefix}")
message("DOWNLOAD_DIR: ${DOWNLOAD_DIR}")
message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message("CORE_BUILD_DIR: ${CORE_BUILD_DIR}")
message("CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
message("ICU_VER: ${ICU_VER}")
message("CORE_SYSTEM_NAME: ${CORE_SYSTEM_NAME}")
message("CORE_PLATFORM_NAME: ${CORE_PLATFORM_NAME}")
message("CPU: ${CPU}")
message("CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message("CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message("ENABLE_CCACHE: ${ENABLE_CCACHE}")
message("CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message("CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
message("CROSS_ARGS: ${CROSS_ARGS}")
endif()

set(CONFIGARCH --arch=${CPU})

if(CORE_SYSTEM_NAME STREQUAL linux)
  set(ICU_CONFIG_PLATFORM Linux)
endif()

if(VERBOSE)
message("ICU_CONFIG_PLATFORM: ${ICU_CONFIG_PLATFORM}")
endif()

# For help on setting configurable options, see runConfigure as well as 
# configure. Both run by this script and located in source for libicu.
#
# Definitions of particular interest: UCONFIG_NO_LEGACY_CONVERSION=1  # Only
# support UTF-7/8/16/32, CESU-8, SCSU, BOCU-1, US-ASCII & ISO-8859-1
#
# Default size of UnicodeString of 64 allows for 27 UTF-16 codeunits to be
# stored in the stack-allocated Unicodestring. Since Kodi uses UnicodeString for
# short-term, go ahead and boost size to 256. If Kodi changed to 
# use UnicodeStrings for long-term, then a smaller size, perhaps 
# the default, will make more sense.
#
# UNISTR_OBJECT_SIZE=64  Default size UNISTR_OBJECT_SIZE=256

if(ENABLE_INTERNAL_LIBICU) # Build our own instead of using system libicu
  set(SYSTEM_ICU OFF)
else()
  set(SYSTEM_ICU ON)
endif()

add_compile_definitions(
  ICU_VER=${ICU_VER}
  # U_DISABLE_RENAMING=0  # Must match ConfigArgs setting. Useful for debugging. 
                        # If set to 1, must specify it to top level CMakeLists as 
                        # -DU_DISBLE_RENAMING=1                        
  U_DEFAULT_SHOW_DRAFT=0 
  U_HIDE_DRAFT_API=1 
  U_HIDE_DEPRECATED_API=1 
  U_HIDE_OBSOLETE_UTF_OLD_H=1  
  U_NO_DEFAULT_INCLUDE_UTF_HEADERS=1  
  UNISTR_FROM_CHAR_EXPLICIT=explicit 
  UNISTR_FROM_STRING_EXPLICIT=explicit  
  UCONFIG_NO_LEGACY_CONVERSION=0 
  UNISTR_OBJECT_SIZE=256 
  SYSTEM_ICU=${SYSTEM_ICU} )

set(ConfigArgs
    --disable-draft
    --disable-extras   
    --enable-icu-config  # A command    
    --disable-layoutex  # Default     
    --disable-samples    
    --enable-strict
    --disable-tests # Disable tests because they depend upon some of the above
                    # options being relaxed
    --enable-tools  # Must have, or keep tools from previous build around
    # Data can be packaged in libicudata.so or in files. Files is recommended,
    # but .so is simpler
    --with-data-packaging=library)

if(DEBUG_ICU)
  	list(APPEND ConfigArgs --enable-debug --disable-release)
else()
  list(APPEND ConfigArgs --disable-debug --enable-release)
endif()

 # By default, every function call is renamed with a version suffix
 # Guarantees that you link with same version you compiled with.
 # Complicates finding symbols with IDE, etc. Note that this setting
 # impacts both the libraries that at created (--enable-renaming) 
 # as well as the functions called by application code
 # (-DU_DISABLE_RENAMING)
 
if(ENABLE_RENAMING)
  list(APPEND ConfigArgs --enable-renaming)
  add_compile_definitions(U_DISABLE_RENAMING=0)
  set(U_DISABLE_RENAMING 0)
else()
  list(APPEND ConfigArgs --disable-renaming)
  add_compile_definitions(U_DISABLE_RENAMING=1)
  set(U_DISABLE_RENAMING 1)
endif()

if(ICU_LIB_TYPE STREQUAL SHARED)
  set(ICU_STATIC NO)
  list(APPEND ConfigArgs --enable-shared)
else()
  set(ICU_STATIC YES)
  list(APPEND ConfigArgs --disable-shared)
endif()

get_property(
  defines
  DIRECTORY
  PROPERTY COMPILE_DEFINITIONS)
  
cmake_print_variables(defines)
cmake_print_variables(ConfigArgs)
string(STRIP ${CMAKE_EXE_LINKER_FLAGS} ld_flags)
string(
  APPEND defines
  \;CFLAGS=\"${CMAKE_C_FLAGS}\"\;CXX_FLAGS=\"${CMAKE_CXX_FLAGS}\"\;LD_FLAGS=\"${ld_flags}\")
if(VERBOSE)
message("concat defines: ${defines}")
endif()

list(APPEND libicu_conf ${CONFIGARCH})

if(VERBOSE)
message(STATUS "STATUS: ${STATUS} libicu_conf: ${libicu_conf}")
message("libicu_conf: ${libicu_conf}")
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

include(ExternalProject)

if(VERBOSE)
message("libicu: ${libicu}")
message("SOURCE_DIR: ${SOURCE_DIR}")
message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}/source")
message("CONFIGURE_COMMAND: ${CONFIGURE_COMMAND}<SOURCE_DIR>/runConfigureICU")
message("ICU_CONFIG_PLATFORM: ${ICU_CONFIG_PLATFORM}")
message("--srcdir=${CMAKE_SOURCE_DIR}/source")
message("--prefix=${CMAKE_INSTALL_PREFIX}")
message("LIBICU_INSTALL_PREFIX: ${LIBICU_INSTALL_PREFIX}")
message("BUILD_COMMAND: ${BUILD_COMMAND} ${MAKE_COMMAND}")

cmake_print_variables(CMAKE_C_FLAGS)
endif()

# Had a lot of trouble figuring out how to properly pass arguments to externalproject_add.
# Quoting did not work as expected. Using -E env helped a lot. Also NOT 
# quoting C_FLAGS, etc, was the way to go. Trying to do things like:
# C_FLAGS="-O0 blah blah" caused real headaches.

set(icu_config_env
    ICU_STATIC=${ICU_STATIC};ICU_VER=${ICU_VER};U_DISABLE_RENAMING=${U_DISABLE_RENAMING};
    [[U_DEFAULT_SHOW_DRAFT=0;U_HIDE_DRAFT_API=1;U_HIDE_DEPRECATED_API=1;U_HIDE_OBSOLETE_UTF_OLD_H=1;]]
    [[U_NO_DEFAULT_INCLUDE_UTF_HEADERS=1;]]
    [[UNISTR_FROM_CHAR_EXPLICIT=explicit;UNISTR_FROM_STRING_EXPLICIT=explicit;]]
    [[UCONFIG_NO_LEGACY_CONVERSION=0;UNISTR_OBJECT_SIZE=256;]])

set(BUILD_ENVIRON CFLAGS=${CMAKE_C_FLAGS} CXX_FLAGS=${CMAKE_CXX_FLAGS} PKG_CONFIG=${PKG_CONFIG_EXECUTABLE} LDFLAGS=${CMAKE_SHARED_LINKER_FLAGS})
list(JOIN ConfigArgs " " config_args_expanded)
list(APPEND BUILD_ENVIRON ${icu_config_env} "SYSTEM_ICU=${SYSTEM_ICU}" )

externalproject_add(libicu
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}/source
                    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env ${BUILD_ENVIRON} <SOURCE_DIR>/runConfigureICU
                      ${ICU_CONFIG_PLATFORM}
                      --srcdir=${CMAKE_SOURCE_DIR}/source
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      ${config_args_expanded} 
                      BUILD_COMMAND )  # Runs make to build after the download and config.

install(CODE "Message(Done)")

# Quell warnings
set(BUILD_SHARED_LIBS)
set(XBMC_BUILD_DIR)
set(KODI_BUILD_DIR)
