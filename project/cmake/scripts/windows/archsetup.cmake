# -------- Architecture settings ---------

set(ARCH win32)


# -------- Paths (mainly for find_package) ---------

set(PLATFORM_DIR win32)

# Precompiled headers fail with per target output directory. (needs CMake 3.1)
set(PRECOMPILEDHEADER_DIR ${PROJECT_BINARY_DIR}/${CORE_BUILD_CONFIG}/objs)

set(CMAKE_SYSTEM_NAME Windows)
list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${PROJECT_SOURCE_DIR}/../../lib/win32)
list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${PROJECT_SOURCE_DIR}/../../lib/win32/ffmpeg)
list(APPEND CMAKE_SYSTEM_LIBRARY_PATH ${PROJECT_SOURCE_DIR}/../../lib/win32/ffmpeg/bin)
list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${PROJECT_SOURCE_DIR}/../BuildDependencies)
set(CONFIGURATION_LIBDIR lib/${CORE_BUILD_CONFIG}-vc120)
set(CONFIGURATION_LIBDIR_RELEASE lib/Release-vc120)
set(CONFIGURATION_LIBDIR_DEBUG lib/Debug-vc120)

set(JPEG_NAMES ${JPEG_NAMES} jpeg-static)
set(PYTHON_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/../BuildDependencies/include/python)


# -------- Compiler options ---------

add_options(CXX ALL_BUILDS "/wd\"4996\"")
set(ARCH_DEFINES -D_WINDOWS -DTARGET_WINDOWS)
set(SYSTEM_DEFINES -DNOMINMAX -D_USE_32BIT_TIME_T -DHAS_DX -D__STDC_CONSTANT_MACROS
                   -DTAGLIB_STATIC -DNPT_CONFIG_ENABLE_LOGGING
                   -DPLT_HTTP_DEFAULT_USER_AGENT="UPnP/1.0 DLNADOC/1.50 Kodi"
                   -DPLT_HTTP_DEFAULT_SERVER="UPnP/1.0 DLNADOC/1.50 Kodi"
                   -DBUILDING_WITH_CMAKE
                   $<$<CONFIG:Debug>:-DD3D_DEBUG_INFO -D_SECURE_SCL=0 -D_HAS_ITERATOR_DEBUGGING=0>)

# Compile with /MT (to be compatible with the dependent libraries)
foreach(CompilerFlag CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                     CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

# Make sure /FS is set for Visual Studio in order to prevent simultanious access to pdb files.
if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set(CMAKE_CXX_FLAGS "/MP /FS ${CMAKE_CXX_FLAGS}")
endif()


# -------- Linker options ---------

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")

# For #pragma comment(lib X)
# TODO: It would certainly be better to handle these libraries via CMake modules.
link_directories(${PROJECT_SOURCE_DIR}/../../lib/win32/ffmpeg/bin
                 ${PROJECT_SOURCE_DIR}/../BuildDependencies/lib
                 ${PROJECT_SOURCE_DIR}/../BuildDependencies/${CONFIGURATION_LIBDIR})

# Additional libraries
list(APPEND DEPLIBS d3d11.lib DInput8.lib DSound.lib winmm.lib Mpr.lib Iphlpapi.lib
                    PowrProf.lib setupapi.lib dwmapi.lib yajl.lib dxguid.lib DelayImp.lib)

# NODEFAULTLIB option
set(_nodefaultlibs_RELEASE libc msvcrt libci msvcprt)
set(_nodefaultlibs_DEBUG libcpmt libc msvcrt libcmt msvcrtd msvcprtd)
foreach(_lib ${_nodefaultlibs_RELEASE})
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:\"${_lib}\"")
endforeach()
foreach(_lib ${_nodefaultlibs_DEBUG})
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:\"${_lib}\"")
endforeach()

# DELAYLOAD option
set(_delayloadlibs libxslt.dll dnssd.dll dwmapi.dll ssh.dll sqlite3.dll
                   avcodec-56.dll avfilter-5.dll avformat-56.dll avutil-54.dll
                   postproc-53.dll swresample-1.dll swscale-3.dll d3dcompiler_47.dll)
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
endif()
