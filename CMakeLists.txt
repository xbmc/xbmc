cmake_minimum_required(VERSION 3.1)
project(kodi LANGUAGES CXX C ASM)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
if(DEPENDS_DIR)
  list(APPEND CMAKE_PREFIX_PATH ${DEPENDS_DIR})
endif()

# Set CORE_BUILD_DIR
set(CORE_BUILD_DIR build)

message(STATUS "Source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")
if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
  message(WARNING "In-source build detected. It is recommended to build out-of-source.")
endif()

# Hide messages from Check*.cmake (e.g. CheckCSourceCompiles)
if(NOT VERBOSE)
  set(CMAKE_REQUIRED_QUIET ON)
endif()

# Includes
include(cmake/modules/extra/ECMEnableSanitizers.cmake)
include(cmake/scripts/common/GeneratorSetup.cmake)
include(cmake/scripts/common/AddOptions.cmake)
include(cmake/scripts/common/ArchSetup.cmake)
include(cmake/scripts/common/Macros.cmake)
include(cmake/scripts/common/ProjectMacros.cmake)
core_find_versions()
include(cmake/scripts/${CORE_SYSTEM_NAME}/PathSetup.cmake)
include(ExternalProject)

# Languages and global compiler settings
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp")

# general
option(VERBOSE            "Enable verbose output?" OFF)
option(ENABLE_DVDCSS      "Enable libdvdcss support?" ON)
option(ENABLE_UPNP        "Enable UPnP support?" ON)
option(ENABLE_AIRTUNES    "Enable AirTunes support?" ON)
option(ENABLE_OPTICAL     "Enable optical support?" ON)
option(ENABLE_PYTHON      "Enable python support?" ON)
# use ffmpeg from depends or system
option(ENABLE_INTERNAL_FFMPEG "Enable internal ffmpeg?" OFF)
if(UNIX)
  option(FFMPEG_PATH        "Path to external ffmpeg?" "")
  option(ENABLE_INTERNAL_CROSSGUID "Enable internal crossguid?" ON)
  option(ENABLE_OPENSSL     "Enable OpenSSL?" ON)
  option(ENABLE_MIR         "Enable MIR?" OFF)
  if(CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd)
    option(ENABLE_X11         "Enable X11 support?" ON)
    option(ENABLE_AML         "Enable AML?" OFF)
    option(ENABLE_IMX         "Enable IMX?" OFF)
  endif()
endif()
# System options
if(NOT WIN32)
  option(WITH_ARCH              "build with given arch" OFF)
  option(WITH_CPU               "build with given cpu" OFF)
endif()
if(CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL rbpi)
  option(ENABLE_LIRC            "Enable LIRC support?" ON)
  option(ENABLE_EVENTCLIENTS    "Enable event clients support?" OFF)
endif()

# Build static libraries per directory
if(NOT CMAKE_GENERATOR MATCHES "Visual Studio" AND NOT CMAKE_GENERATOR STREQUAL Xcode)
  set(ENABLE_STATIC_LIBS TRUE)
else()
  set(ENABLE_STATIC_LIBS FALSE)
endif()

core_find_git_rev(APP_SCMID FULL)

# Dynamically loaded libraries built with the project
add_custom_target(${APP_NAME_LC}-libraries)
set(LIBRARY_FILES "" CACHE STRING "" FORCE)
mark_as_advanced(LIBRARY_FILES)

set(INCLUDES ${CMAKE_SOURCE_DIR}
             ${CMAKE_SOURCE_DIR}/lib
             ${CMAKE_SOURCE_DIR}/lib/gtest/include
             ${CMAKE_SOURCE_DIR}/xbmc
             ${CMAKE_SOURCE_DIR}/xbmc/${PLATFORM_DIR}
             ${CMAKE_SOURCE_DIR}/xbmc/cores/VideoPlayer
             ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR})

find_package(PkgConfig)
if(KODI_DEPENDSBUILD)
  if(PKG_CONFIG_EXECUTABLE)
    set(PKG_CONFIG_FOUND TRUE)
  endif()
endif()

find_package(Threads REQUIRED QUIET)
list(APPEND DEPLIBS ${CMAKE_THREAD_LIBS_INIT})

# Required dependencies. Keep in alphabetical order please
set(required_deps Cdio
                  Cpluff
                  CrossGUID
                  FFMPEG
                  Fmt
                  FreeType
                  FriBidi
                  LibDvd
                  Lzo2
                  PCRE
                  RapidJSON
                  Sqlite3
                  TagLib
                  TinyXML)
if(NOT WIN32)
  list(APPEND required_deps ZLIB)
else()
  list(APPEND required_deps D3DX11Effects)
endif()
if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND required_deps Zip)
endif()

# Optional dependencies. Keep in alphabetical order please
set(optional_deps Alsa
                  Avahi
                  Bluetooth
                  CAP
                  CCache
                  DBus
                  LCMS2
                  MDNS
                  MicroHttpd
                  MySqlClient
                  PulseAudio
                  Python
                  SmbClient
                  SSH
                  UDEV
                  VAAPI
                  VDPAU
                  XSLT)

# Required, dyloaded deps. Keep in alphabetical order please
set(required_dyload ASS
                    Curl)

# Optional, dyloaded deps. Keep in alphabetical order please
set(dyload_optional Bluray
                    CEC
                    NFS
                    Plist)

# Required by shared objects we link
set(required_dep_libs EXPAT)

# Required tools
find_package(TexturePacker REQUIRED)
find_package(JsonSchemaBuilder REQUIRED)

foreach(dep ${required_dep_libs})
  find_package(${dep} REQUIRED)
endforeach()

foreach(dep ${required_deps})
  core_require_dep(${dep})
endforeach()

foreach(dep ${optional_deps})
  core_optional_dep(${dep})
endforeach()

if(NOT UDEV_FOUND)
  core_optional_dep(LibUSB)
endif()

foreach(dep ${required_dyload})
  core_require_dyload_dep(${dep})
endforeach()

foreach(dep ${dyload_optional})
  core_optional_dyload_dep(${dep})
endforeach()

if(ENABLE_OPENSSL)
  core_require_dep(OpenSSL)
  list(APPEND DEP_DEFINES "-DHAVE_OPENSSL=1")
endif()

if(ENABLE_UPNP)
  list(APPEND DEP_DEFINES "-DUSE_UPNP=1")
endif()

if(ENABLE_OPTICAL)
  list(APPEND DEP_DEFINES -DHAS_DVD_DRIVE)
endif()

if(ENABLE_LIRC)
  set(LIRC_DEVICE /dev/lircd CACHE STRING "LIRC device to use")
  list(APPEND DEP_DEFINES -DLIRC_DEVICE="${LIRC_DEVICE}" -DHAVE_LIRC=1)
endif()

if(ENABLE_AIRTUNES)
  find_package(Shairplay)
  if(SHAIRPLAY_FOUND)
    core_require_dyload_dep(Shairplay)
  endif()
endif()

if(CORE_SYSTEM_NAME STREQUAL osx)
  core_require_dep(Sdl)
endif()

if(NOT WIN32)
  core_optional_dep(OpenGl)
  if(OPENGL_FOUND AND NOT APPLE)
    if(ENABLE_MIR)
      core_require_dep(Mir ENABLE_MIR)
      core_optional_dep(LibDRM ENABLE_MIR)
    else()
      core_optional_dep(X ENABLE_X11)
      core_optional_dep(XRandR ENABLE_X11)
      core_optional_dep(LibDRM ENABLE_X11)
    endif()
  else()
    core_optional_dep(OpenGLES)
    if(OPENGLES_FOUND)
      if(ENABLE_MIR)
        core_require_dep(Mir ENABLE_MIR)
        core_optional_dep(LibDRM ENABLE_MIR)
      else()
        core_optional_dep(X ENABLE_X11)
        core_optional_dep(XRandR ENABLE_X11)
        core_optional_dep(LibDRM ENABLE_X11)
      endif()
    endif()
  endif()
  if(NOT APPLE)
    core_require_dep(EGL)
  endif()
endif()

if(CORE_SYSTEM_NAME STREQUAL android OR CORE_SYSTEM_NAME STREQUAL linux)
  core_optional_dep(AML)
  core_optional_dep(IMX)
elseif(CORE_SYSTEM_NAME STREQUAL rbpi)
  core_require_dep(MMAL)
endif()

# Compile Info
add_custom_command(OUTPUT ${CORE_BUILD_DIR}/xbmc/CompileInfo.cpp
                          ${CMAKE_BINARY_DIR}/addons/xbmc.addon/addon.xml
                          ${CMAKE_BINARY_DIR}/addons/kodi.guilib/addon.xml
                   COMMAND ${CMAKE_COMMAND} -DCORE_SOURCE_DIR=${CMAKE_SOURCE_DIR}
                                            -DCORE_SYSTEM_NAME=${CORE_SYSTEM_NAME}
                                            -DCORE_BUILD_DIR=${CORE_BUILD_DIR}
                                            -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
                                            -DARCH_DEFINES="${ARCH_DEFINES}"
                                            -DAPP_SCMID=${APP_SCMID}
                                            -Dprefix=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                                            -P ${CMAKE_SOURCE_DIR}/cmake/scripts/common/GenerateVersionedFiles.cmake
                   DEPENDS ${CMAKE_SOURCE_DIR}/version.txt
                           ${CMAKE_SOURCE_DIR}/addons/xbmc.addon/addon.xml.in
                           ${CMAKE_SOURCE_DIR}/addons/kodi.guilib/addon.xml.in
                           ${CMAKE_SOURCE_DIR}/xbmc/CompileInfo.cpp.in)
list(APPEND install_data addons/xbmc.addon/addon.xml)
list(APPEND install_data addons/xbmc.json/addon.xml)
list(APPEND install_data addons/kodi.guilib/addon.xml)
add_library(compileinfo OBJECT ${CORE_BUILD_DIR}/xbmc/CompileInfo.cpp)
set_target_properties(compileinfo PROPERTIES FOLDER "Build Utilities")
target_compile_options(compileinfo PRIVATE "${SYSTEM_DEFINES}")

# RC File
if(WIN32)
  configure_file(${CMAKE_SOURCE_DIR}/xbmc/platform/win32/XBMC_PC.rc.in
                 ${CORE_BUILD_DIR}/xbmc/platform/win32/XBMC_PC.rc @ONLY)
  add_library(resources OBJECT ${CORE_BUILD_DIR}/xbmc/platform/win32/XBMC_PC.rc)
  set_target_properties(resources PROPERTIES FOLDER "Build Utilities")
  target_include_directories(resources PRIVATE ${CMAKE_SOURCE_DIR}/tools/windows/packaging/media)
  set(RESOURCES $<TARGET_OBJECTS:resources>)
endif()

include_directories(${INCLUDES} ${SYSTEM_INCLUDES})
add_compile_options(${ARCH_DEFINES} "${SYSTEM_DEFINES}" ${DEP_DEFINES} ${PATH_DEFINES})

set(core_DEPENDS "" CACHE STRING "" FORCE)
set(test_archives "" CACHE STRING "" FORCE)
set(test_sources "" CACHE STRING "" FORCE)
mark_as_advanced(core_DEPENDS)
mark_as_advanced(test_archives)
mark_as_advanced(test_sources)

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/gtest ${CORE_BUILD_DIR}/gtest EXCLUDE_FROM_ALL)
set_target_properties(gtest PROPERTIES FOLDER "External Projects")

# copy files to build tree
copy_files_from_filelist_to_buildtree(${CMAKE_SOURCE_DIR}/cmake/installdata/common/*.txt
                                      ${CMAKE_SOURCE_DIR}/cmake/installdata/${CORE_SYSTEM_NAME}/*.txt)

list(APPEND SKINS "${CMAKE_SOURCE_DIR}/addons/skin.estuary\;${CMAKE_SOURCE_DIR}")
list(APPEND SKINS "${CMAKE_SOURCE_DIR}/addons/skin.estouchy\;${CMAKE_SOURCE_DIR}")

# These are skins that are copied into place from the source tree
foreach(skin ${SKINS})
  list(GET skin 0 dir)
  list(GET skin 1 relative)
  copy_skin_to_buildtree(${dir} ${relative})
endforeach()

add_custom_target(pack-skins ALL
                  DEPENDS TexturePacker::TexturePacker export-files ${XBT_FILES})
set_target_properties(pack-skins PROPERTIES FOLDER "Build Utilities")

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/system/players/VideoPlayer)

set(LIBCEC_SONAME "${CEC_SONAME}")
if(NOT CORE_SYSTEM_NAME STREQUAL windows AND NOT CORE_SYSTEM_NAME STREQUAL android)
  configure_file(${CMAKE_SOURCE_DIR}/xbmc/DllPaths_generated.h.in
                 ${CORE_BUILD_DIR}/DllPaths_generated.h @ONLY)
elseif(CORE_SYSTEM_NAME STREQUAL android)
  configure_file(${CMAKE_SOURCE_DIR}/xbmc/DllPaths_generated_android.h.in
                 ${CORE_BUILD_DIR}/DllPaths_generated_android.h @ONLY)
endif()

# main library (used for main binary and tests)
add_library(lib${APP_NAME_LC} STATIC $<TARGET_OBJECTS:compileinfo>)
set_target_properties(lib${APP_NAME_LC} PROPERTIES PREFIX "")
add_dependencies(lib${APP_NAME_LC} libcpluff ffmpeg dvdnav crossguid)

# Other files (IDE)
set(OTHER_FILES cmake/README.md)

# Subdirs
core_add_subdirs_from_filelist(${CMAKE_SOURCE_DIR}/cmake/treedata/common/*.txt
                               ${CMAKE_SOURCE_DIR}/cmake/treedata/${CORE_SYSTEM_NAME}/*.txt)
core_add_optional_subdirs_from_filelist(${CMAKE_SOURCE_DIR}/cmake/treedata/optional/common/*.txt
                                        ${CMAKE_SOURCE_DIR}/cmake/treedata/optional/${CORE_SYSTEM_NAME}/*.txt)

target_link_libraries(lib${APP_NAME_LC} PUBLIC ${core_DEPENDS} ${SYSTEM_LDFLAGS} ${DEPLIBS} ${CMAKE_DL_LIBS})
set_target_properties(lib${APP_NAME_LC} PROPERTIES PROJECT_LABEL "xbmc")
source_group_by_folder(lib${APP_NAME_LC} RELATIVE ${CMAKE_SOURCE_DIR}/xbmc)
if(WIN32)
  add_precompiled_header(lib${APP_NAME_LC} pch.h ${CMAKE_SOURCE_DIR}/xbmc/platform/win32/pch.cpp PCH_TARGET kodi)
  set_language_cxx(lib${APP_NAME_LC})
endif()

# main binary
if(NOT CORE_SYSTEM_NAME STREQUAL android)
  add_executable(${APP_NAME_LC} ${CORE_MAIN_SOURCE} "${RESOURCES}" ${OTHER_FILES})
  set_target_properties(${APP_NAME_LC} PROPERTIES ENABLE_EXPORTS ON)
else()
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
  add_library(${APP_NAME_LC} SHARED ${CORE_MAIN_SOURCE} "${RESOURCES}" ${OTHER_FILES})
endif()
add_dependencies(${APP_NAME_LC} ${APP_NAME_LC}-libraries export-files pack-skins)
whole_archive(_MAIN_LIBRARIES ${core_DEPENDS})
target_link_libraries(${APP_NAME_LC} ${_MAIN_LIBRARIES} lib${APP_NAME_LC} ${DEPLIBS})
unset(_MAIN_LIBRARIES)

if(WIN32)
  set_target_properties(${APP_NAME_LC} PROPERTIES WIN32_EXECUTABLE ON)
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${APP_NAME_LC})
  target_sources(${APP_NAME_LC} PRIVATE ${CMAKE_SOURCE_DIR}/xbmc/platform/win32/app.manifest)
elseif(CORE_SYSTEM_NAME STREQUAL android)
  # Nothing
elseif(CORE_SYSTEM_NAME STREQUAL ios)
  set_target_properties(${APP_NAME_LC} PROPERTIES OUTPUT_NAME ${APP_NAME}
                                                  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/ios/Info.plist.in)
else()
  set_target_properties(${APP_NAME_LC} PROPERTIES SUFFIX ".bin")
endif()

# testing
copy_files_from_filelist_to_buildtree(${CMAKE_SOURCE_DIR}/cmake/installdata/test-reference-data.txt NO_INSTALL)
add_executable(${APP_NAME_LC}-test EXCLUDE_FROM_ALL ${CMAKE_SOURCE_DIR}/xbmc/test/xbmc-test.cpp ${test_sources})
whole_archive(_TEST_LIBRARIES ${core_DEPENDS} gtest)
target_link_libraries(${APP_NAME_LC}-test PRIVATE ${SYSTEM_LDFLAGS} ${_TEST_LIBRARIES} lib${APP_NAME_LC} ${DEPLIBS} ${CMAKE_DL_LIBS})
unset(_TEST_LIBRARIES)
add_dependencies(${APP_NAME_LC}-test ${APP_NAME_LC}-libraries export-files)
if(WIN32)
  add_precompiled_header(${APP_NAME_LC}-test pch.h ${CMAKE_SOURCE_DIR}/xbmc/platform/win32/pch.cpp PCH_TARGET kodi)
endif()

# Enable unit-test related targets
if(CORE_HOST_IS_TARGET)
  enable_testing()
  gtest_add_tests(${APP_NAME_LC}-test "" ${test_sources})
  add_custom_target(check ${CMAKE_CTEST_COMMAND} WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
  add_dependencies(check ${APP_NAME_LC}-test)

  # Valgrind (memcheck)
  find_program(VALGRIND_EXECUTABLE NAMES valgrind)
  if(VALGRIND_EXECUTABLE)
    set(CTEST_MEMORYCHECK_COMMAND ${VALGRIND_EXECUTABLE})
    set(CTEST_MEMORYCHECK_COMMAND_OPTIONS "-q --trace-children=yes --leak-check=yes --track-origins=yes")
    include(CTest)
    add_custom_target(check-valgrind ${CMAKE_CTEST_COMMAND} -D ExperimentalMemCheck \${ARGS} WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
    add_dependencies(check-valgrind ${APP_NAME_LC}-test)
  endif()

  # For testing commit series
  add_custom_target(check-commits ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/scripts/common/CheckCommits.cmake
                                                   -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR})
  set_target_properties(check-commits PROPERTIES FOLDER "Build Utilities")

  # code coverage
  if(CMAKE_BUILD_TYPE STREQUAL Coverage)
    if(EXISTS ${CMAKE_SOURCE_DIR}/scripts/${CORE_SYSTEM_NAME}/CodeCoverage.cmake)
      include(${CMAKE_SOURCE_DIR}/scripts/${CORE_SYSTEM_NAME}/CodeCoverage.cmake)
    else()
      message(FATAL_ERROR "Code coverage not (yet) implemented for platform ${CORE_SYSTEM_NAME}")
    endif()
  endif()

  # Documentation
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    add_custom_target(doc
                      COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/doxygen_resources/Doxyfile.doxy
                      COMMAND ${CMAKE_COMMAND} -E echo "Documentation built to: file://${CMAKE_SOURCE_DIR}/docs/html/index.html"
                      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/doxygen_resources
                      COMMENT "Generating Doxygen documentation" VERBATIM)
  endif()
endif()

# link wrapper
if(FFMPEG_LINK_EXECUTABLE)
  set(CMAKE_CXX_LINK_EXECUTABLE "${FFMPEG_LINK_EXECUTABLE}")
endif()
if(FFMPEG_CREATE_SHARED_LIBRARY)
  set(CMAKE_CXX_CREATE_SHARED_LIBRARY "${FFMPEG_CREATE_SHARED_LIBRARY}")
endif()

# Platform specific additional extra targets
if(EXISTS ${CMAKE_SOURCE_DIR}/cmake/scripts/${CORE_SYSTEM_NAME}/ExtraTargets.cmake)
  include(${CMAKE_SOURCE_DIR}/cmake/scripts/${CORE_SYSTEM_NAME}/ExtraTargets.cmake)
endif()

include(cmake/scripts/${CORE_SYSTEM_NAME}/Install.cmake)

# Add uninstall target
if(CMAKE_GENERATOR MATCHES Makefile)
  add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/scripts/common/Uninstall.cmake)
  set_target_properties(uninstall PROPERTIES FOLDER "Build Utilities")
endif()

# Create target that allows to build binary-addons.
# Use make binary-addons ADDONS="visualization.spectrum" to select the addons to build.
if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
  if(CMAKE_CROSSCOMPILING)
    set(_cross_args CROSS_COMPILING=yes)
  endif()
  if(NOT CORE_SYSTEM_NAME STREQUAL android)
    set(_prefix ${CMAKE_BINARY_DIR}/addons)
    set(_extra "CMAKE_EXTRA=-DPACKAGE_ZIP=ON")
  else()
    set(_prefix ${CMAKE_INSTALL_PREFIX})
  endif()
  add_custom_target(binary-addons
    COMMAND $(MAKE) -C ${CMAKE_SOURCE_DIR}/tools/depends/target/binary-addons clean
    COMMAND $(MAKE) -C ${CMAKE_SOURCE_DIR}/tools/depends/target/binary-addons VERBOSE=1 V=99
                    PREFIX=${_prefix} INSTALL_PREFIX=${_prefix} ${_cross_args} ${_extra})
  unset(_cross_args)
  unset(_prefix)
endif()

# Prepare add-on build env
core_file_read_filtered(bindings ${CMAKE_SOURCE_DIR}/xbmc/addons/addon-bindings.mk)
foreach(header ${bindings})
  get_filename_component(file ${header} NAME)
  configure_file(${CMAKE_SOURCE_DIR}/${header} ${CORE_BUILD_DIR}/include/${APP_NAME_LC}/${file} COPYONLY)
endforeach()

set(APP_LIB_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/${APP_NAME_LC})
set(APP_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include/${APP_NAME_LC})
set(CXX11_SWITCH "-std=c++11")
configure_file(${CMAKE_SOURCE_DIR}/cmake/KodiConfig.cmake.in
               ${CORE_BUILD_DIR}/lib/${APP_NAME_LC}/${APP_NAME}Config.cmake @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/common/AddonHelpers.cmake
               ${CORE_BUILD_DIR}/lib/${APP_NAME_LC}/AddonHelpers.cmake COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/common/AddOptions.cmake
               ${CORE_BUILD_DIR}/lib/${APP_NAME_LC}/AddOptions.cmake COPYONLY)

# Status
message(STATUS "#---- CONFIGURATION ----#")
foreach(msg ${final_message})
  message(STATUS ${msg})
endforeach()

if(VERBOSE)
  message(STATUS "\n#--------------- Internal Variables -------------#")
  message(STATUS "DEPLIBS: ${DEPLIBS}")
  message(STATUS "core_DEPENDS: ${core_DEPENDS}")
  message(STATUS "#---------------------------------------------#")
  message(STATUS "SYSTEM_LDFLAGS        : ${SYSTEM_LDFLAGS}")
  message(STATUS "CMAKE_DL_LIBS         : ${CMAKE_DL_LIBS}")
  message(STATUS "CMAKE_C_FLAGS         : ${CMAKE_C_FLAGS}")
  message(STATUS "CMAKE_CXX_FLAGS       : ${CMAKE_CXX_FLAGS}")
  message(STATUS "CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
  message(STATUS "#---------------------------------------------#")
  message(STATUS "bindir     : ${bindir}")
  message(STATUS "includedir : ${includedir}")
  message(STATUS "libdir     : ${libdir}")
  message(STATUS "datarootdir: ${datarootdir}")
  message(STATUS "#---------------------------------------------#")
  message(STATUS "GIT_REV: ${APP_SCMID}")
  message(STATUS "#---------------------------------------------#")
  message(STATUS "CPACK_GENERATOR       : ${CPACK_GENERATOR}")
  message(STATUS "CPACK_SOURCE_GENERATOR: ${CPACK_SOURCE_GENERATOR}")
  message(STATUS "#---------------------------------------------#")
endif()
