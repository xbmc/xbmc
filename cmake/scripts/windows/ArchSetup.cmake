# Minimum SDK version we support
set(VS_MINIMUM_SDK_VERSION 10.0.14393.0)

if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION VERSION_LESS VS_MINIMUM_SDK_VERSION)
  message(FATAL_ERROR "Detected Windows SDK version is ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}.\n"
    "Windows SDK ${VS_MINIMUM_SDK_VERSION} or higher is required.\n"
    "INFO: Windows SDKs can be installed from the Visual Studio installer.")
endif()

# -------- Architecture settings ---------

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(ARCH win32)
  set(SDK_TARGET_ARCH x86)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(ARCH x64)
  set(SDK_TARGET_ARCH x64)
endif()


# -------- Paths (mainly for find_package) ---------

set(PLATFORM_DIR platform/win32)
set(APP_RENDER_SYSTEM dx11)

set(CORE_MAIN_SOURCE ${CMAKE_SOURCE_DIR}/xbmc/platform/win32/WinMain.cpp)

# Precompiled headers fail with per target output directory. (needs CMake 3.1)
set(PRECOMPILEDHEADER_DIR ${PROJECT_BINARY_DIR}/${CORE_BUILD_CONFIG}/objs)
set(CMAKE_SYSTEM_NAME Windows)
set(DEPS_FOLDER_RELATIVE project/BuildDependencies)
set(DEPENDENCIES_DIR ${CMAKE_SOURCE_DIR}/${DEPS_FOLDER_RELATIVE}/${ARCH})
set(MINGW_LIBS_DIR ${CMAKE_SOURCE_DIR}/${DEPS_FOLDER_RELATIVE}/mingwlibs/${ARCH})

list(APPEND CMAKE_PREFIX_PATH ${MINGW_LIBS_DIR})
list(APPEND CMAKE_LIBRARY_PATH ${MINGW_LIBS_DIR}/bin)
list(APPEND CMAKE_PREFIX_PATH ${DEPENDENCIES_DIR})

# -------- Compiler options ---------

add_options(CXX ALL_BUILDS "/wd\"4996\"")
set(ARCH_DEFINES -D_WINDOWS -DTARGET_WINDOWS -DTARGET_WINDOWS_DESKTOP -D__SSE__ -D__SSE2__)
set(SYSTEM_DEFINES -DWIN32_LEAN_AND_MEAN -DNOMINMAX -DHAS_DX -D__STDC_CONSTANT_MACROS
                   -DTAGLIB_STATIC -DNPT_CONFIG_ENABLE_LOGGING
                   -DPLT_HTTP_DEFAULT_USER_AGENT="UPnP/1.0 DLNADOC/1.50 Kodi"
                   -DPLT_HTTP_DEFAULT_SERVER="UPnP/1.0 DLNADOC/1.50 Kodi"
                   -DUNICODE -D_UNICODE
                   $<$<CONFIG:Debug>:-DD3D_DEBUG_INFO>)

if(${ARCH} STREQUAL win32)
  list(APPEND SYSTEM_DEFINES $<$<CONFIG:Debug>:-D_ITERATOR_DEBUG_LEVEL=0>)
endif()

# Additional SYSTEM_DEFINES
list(APPEND SYSTEM_DEFINES -DHAS_WIN32_NETWORK -DHAS_FILESYSTEM_SMB)

# Make sure /FS is set for Visual Studio in order to prevent simultaneous access to pdb files.
if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set(CMAKE_CXX_FLAGS "/MP /FS ${CMAKE_CXX_FLAGS}")
endif()

# Google Test needs to use shared version of runtime libraries
set(gtest_force_shared_crt ON CACHE STRING "" FORCE)


# -------- Linker options ---------

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")

# For #pragma comment(lib X)
# TODO: It would certainly be better to handle these libraries via CMake modules.
link_directories(${DEPENDENCIES_DIR}/lib)

# Additional libraries
list(APPEND DEPLIBS bcrypt.lib d3d11.lib DInput8.lib DSound.lib winmm.lib Mpr.lib Iphlpapi.lib WS2_32.lib
                    PowrProf.lib setupapi.lib Shlwapi.lib dwmapi.lib dxguid.lib DelayImp.lib Mincore.lib)

# NODEFAULTLIB option
set(_nodefaultlibs_RELEASE libcmt)
set(_nodefaultlibs_DEBUG libcmt msvcrt)
foreach(_lib ${_nodefaultlibs_RELEASE})
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:\"${_lib}\"")
endforeach()
foreach(_lib ${_nodefaultlibs_DEBUG})
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:\"${_lib}\"")
endforeach()

# DELAYLOAD option
set(_delayloadlibs zlib.dll libmysql.dll libxslt.dll dnssd.dll dwmapi.dll sqlite3.dll
                   d3dcompiler_47.dll)
foreach(_lib ${_delayloadlibs})
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:\"${_lib}\"")
endforeach()

# Make the Release version create a PDB
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
# Minimize the size or the resulting DLLs
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF")


# -------- Visual Studio options ---------

if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  # Generate a batch file that opens Visual Studio with the necessary env variables set.
  file(WRITE ${CMAKE_BINARY_DIR}/kodi-sln.bat
             "@echo off\n"
             "set KODI_HOME=%~dp0\n"
             "set PATH=%~dp0\\system\n"
             "set PreferredToolArchitecture=x64\n"
             "start %~dp0\\${PROJECT_NAME}.sln")
endif()
