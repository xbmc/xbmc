set(dependdir ${root}/project/BuildDependencies)

######################### Compiler CFLAGS
if(MSVC)
  set(CMAKE_C_FLAGS_DEBUG "/Zi /MP /Od /Oy- /D_DEBUG /Gm- /MTd /GS /arch:SSE /fp:precise /Zc:wchar_t /Zc:forScope /wd\"4996\"")
  set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})

  set(CMAKE_C_FLAGS_RELEASE "/DNDEBUG /WX- /MP /Ox /Ot /Oy /GF- /Gm- /EHa /MT /GS /Gy- /arch:SSE /fp:precise /Zc:wchar_t /Zc:forScope /wd\"4996\"")
  set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

  set(CMAKE_CXX_FLAGS "/DWIN32 /D_WINDOWS /W3 /Zm1000 /GR /EHa")
endif(MSVC)

######################### Linker flags
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x86" ${dependdir}/lib)
set(ENV{LIBS} "$ENV{LIBS};${dependdir}/lib")
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:\"libc\" /NODEFAULTLIB:\"msvcrt\" /NODEFAULTLIB:\"libcmt\" /NODEFAULTLIB:\"msvcrtd\" /NODEFAULTLIB:\"msvcprtd\" /DELAYLOAD:\"dnssd.dll\" /DELAYLOAD:\"dwmapi.dll\" /DELAYLOAD:\"libmicrohttpd-5.dll\" /DELAYLOAD:\"ssh.dll\" /DELAYLOAD:\"sqlite3.dll\" /DELAYLOAD:\"libsamplerate-0.dll\"  /SUBSYSTEM:WINDOWS /LARGEADDRESSAWARE /TLBID:1 /DYNAMICBASE /NXCOMPAT /MACHINE:X86 /ERRORREPORT:QUEUE")
else()
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:\"libc\" /NODEFAULTLIB:\"msvcrt\" /NODEFAULTLIB:\"libci\" /NODEFAULTLIB:\"msvcprt\" /DELAYLOAD:\"dnssd.dll\" /DELAYLOAD:\"dwmapi.dll\" /DELAYLOAD:\"libmicrohttpd-5.dll\" /DELAYLOAD:\"ssh.dll\" /DELAYLOAD:\"sqlite3.dll\" /DELAYLOAD:\"libsamplerate-0.dll\"  /SUBSYSTEM:WINDOWS /LARGEADDRESSAWARE /TLBID:1 /DYNAMICBASE /NXCOMPAT /MACHINE:X86 /ERRORREPORT:QUEUE")
endif()

set(system_libs
  D3dx9
  DInput8
  DSound
  winmm
  Mpr
  Iphlpapi
  PowrProf
  setupapi
  dwmapi
  yajl
  dxguid
  DxErr
  Delayimp
  opengl32
  glu32
)

set(external_libs
  libfribidi
  libiconv
  turbojpeg-static
  libmicrohttpd.dll
  freetype246MT
  sqlite3
  liblzo2
  dnssd
  libcdio.dll
  zlib
  libsamplerate-0
)

set(non_link_libs
  SDL
  SDL_image
  fontconfig
)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(external_libs ${external_libs} tinyxmlSTLd libboost_thread-vc100-mt-sgd-1_46_1 libboost_system-vc100-mt-sgd-1_46_1)
else()
  set(external_libs ${external_libs} tinyxmlSTL libboost_thread-vc100-mt-s-1_46_1 libboost_system-vc100-mt-s-1_46_1)
endif()

foreach(lib ${external_libs})
  plex_find_library(${lib} 0 1 ${dependdir}/lib 1)
endforeach()

foreach(lib ${non_link_libs})
  plex_find_library(${lib} 0 1 ${dependdir}/lib 0)
endforeach()

foreach(lib ${system_libs})
  plex_find_library(${lib} 0 0 "" 1)
endforeach()

set(CONFIG_LIBRARY_OPENGL ${CONFIG_LIBRARY_OPENGL32} ${CONFIG_LIBRARY_GLU32})

set(ARCH "x86-win")
set(EXECUTABLE_NAME "Plex Home Theater")
set(FFMPEG_INCLUDE_DIRS ${dependdir}/include)
set(LIBPATH .)
set(BINPATH .)
set(RESOURCEPATH .)



############## definitions
add_definitions(
  -DTARGET_WINDOWS
  -D_WINDOWS
  -D_MSVC
  -DWIN32
  -D_WIN32_WINNT=0x0501
  -DNTDDI_VERSION=0x05010300
  -DNOMINMAX
  -D_USE_32BIT_TIME_T
  -DHAS_DX
  -DD3D_DEBUG_INFO
  -D__STDC_CONSTANT_MACROS
  -D_SECURE_SCL=0
  -D_HAS_ITERATOR_DEBUGGING=0
  -DTAGLIB_STATIC
  -DBOOST_ALL_NO_LIB
  -DBOOST_THREAD_USE_LIB
)