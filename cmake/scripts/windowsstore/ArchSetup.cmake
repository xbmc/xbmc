# Minimum SDK version we support
set(VS_MINIMUM_SDK_VERSION 10.0.18362.0)

if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION VERSION_LESS VS_MINIMUM_SDK_VERSION)
  message(FATAL_ERROR "Detected Windows SDK version is ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}.\n"
    "Windows SDK ${VS_MINIMUM_SDK_VERSION} or higher is required.\n"
    "INFO: Windows SDKs can be installed from the Visual Studio installer.")
endif()

# -------- Host Settings ---------

set(_gentoolset ${CMAKE_GENERATOR_TOOLSET})
string(REPLACE "host=" "" HOSTTOOLSET ${_gentoolset})
unset(_gentoolset)

# -------- Architecture settings ---------

check_symbol_exists(_X86_ "Windows.h" _X86_)
check_symbol_exists(_AMD64_ "Windows.h" _AMD64_)
check_symbol_exists(_ARM_ "Windows.h" _ARM_)

if(_X86_)
   set(ARCH win32)
   set(SDK_TARGET_ARCH x86)
elseif(_AMD64_)
   set(ARCH x64)
   set(SDK_TARGET_ARCH x64)
elseif(_ARM_)
   set(ARCH arm)
   set(SDK_TARGET_ARCH arm)
else()
   message(FATAL_ERROR "Unsupported architecture")
endif()

unset(_X86_)
unset(_AMD64_)
unset(_ARM_)

# -------- Paths (mainly for find_package) ---------

set(PLATFORM_DIR platform/win32)
set(APP_RENDER_SYSTEM dx11)
set(CORE_MAIN_SOURCE ${CMAKE_SOURCE_DIR}/xbmc/platform/win10/main.cpp)

# Precompiled headers fail with per target output directory. (needs CMake 3.1)
set(PRECOMPILEDHEADER_DIR ${PROJECT_BINARY_DIR}/${CORE_BUILD_CONFIG}/objs)

set(CMAKE_SYSTEM_NAME WindowsStore)
set(CORE_SYSTEM_NAME "windowsstore")
set(PACKAGE_GUID "281d668b-5739-4abd-b3c2-ed1cda572ed2")
set(APP_MANIFEST_NAME package.appxmanifest)
set(DEPS_FOLDER_RELATIVE project/BuildDependencies)

# ToDo: currently host build tools are hardcoded to win32
# If we ever allow package.native other than 0_package.native-win32.list we will want to
# adapt this based on host
set(NATIVEPREFIX ${CMAKE_SOURCE_DIR}/${DEPS_FOLDER_RELATIVE}/win32)
set(DEPENDS_PATH ${CMAKE_SOURCE_DIR}/${DEPS_FOLDER_RELATIVE}/win10-${ARCH})
set(MINGW_LIBS_DIR ${CMAKE_SOURCE_DIR}/${DEPS_FOLDER_RELATIVE}/mingwlibs/win10-${ARCH})

# mingw libs
list(APPEND CMAKE_PREFIX_PATH ${MINGW_LIBS_DIR})
list(APPEND CMAKE_LIBRARY_PATH ${MINGW_LIBS_DIR}/bin)

if(NOT TARBALL_DIR)
  set(TARBALL_DIR "${CMAKE_SOURCE_DIR}/project/BuildDependencies/downloads")
endif()

# -------- Compiler options ---------

add_options(CXX ALL_BUILDS "/wd\"4996\"")
add_options(CXX ALL_BUILDS "/wd\"4146\"")
add_options(CXX ALL_BUILDS "/wd\"4251\"")
add_options(CXX ALL_BUILDS "/wd\"4668\"")
add_options(CXX ALL_BUILDS "/wd\"5033\"")
set(ARCH_DEFINES -D_WINDOWS -DTARGET_WINDOWS -DTARGET_WINDOWS_STORE -DXBMC_EXPORT -DMS_UWP -DMS_STORE)
if(NOT SDK_TARGET_ARCH STREQUAL arm)
  list(APPEND ARCH_DEFINES -D__SSE__ -D__SSE2__)
endif()
set(SYSTEM_DEFINES -DWIN32_LEAN_AND_MEAN -DNOMINMAX -DHAS_DX -D__STDC_CONSTANT_MACROS
                   -DTAGLIB_STATIC -DNPT_CONFIG_ENABLE_LOGGING
                   -DPLT_HTTP_DEFAULT_USER_AGENT="UPnP/1.0 DLNADOC/1.50 Kodi"
                   -DPLT_HTTP_DEFAULT_SERVER="UPnP/1.0 DLNADOC/1.50 Kodi"
                   -DUNICODE -D_UNICODE
                   -DFRIBIDI_STATIC
                   $<$<CONFIG:Debug>:-DD3D_DEBUG_INFO>)

# Additional SYSTEM_DEFINES
list(APPEND SYSTEM_DEFINES -DHAS_WIN10_NETWORK)

# The /MP option enables /FS by default.
if(DEFINED ENV{MAXTHREADS})
  set(MP_FLAG "/MP$ENV{MAXTHREADS}")
else()
  set(MP_FLAG "/MP")
endif()
set(CMAKE_CXX_FLAGS "${MP_FLAG} ${CMAKE_CXX_FLAGS} /EHsc /await /permissive-")
# Google Test needs to use shared version of runtime libraries
set(gtest_force_shared_crt ON CACHE STRING "" FORCE)


# -------- Linker options ---------

# For #pragma comment(lib X)
# TODO: It would certainly be better to handle these libraries via CMake modules.
link_directories(${MINGW_LIBS_DIR}/lib
                 ${DEPENDS_PATH}/lib)

list(APPEND DEPLIBS bcrypt.lib d3d11.lib WS2_32.lib dxguid.lib dloadhelper.lib WindowsApp.lib)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /WINMD:NO")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt /DEBUG:FASTLINK /OPT:NOREF /OPT:NOICF")

# Make the Release version create a PDB
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
# Minimize the size or the resulting DLLs
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF")
# remove warning
set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4264")


# -------- Visual Studio options ---------

if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

# -------- Build options ---------

set(ENABLE_OPTICAL OFF CACHE BOOL "" FORCE)
