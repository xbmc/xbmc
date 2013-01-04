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
)

set(external_libs
  libfribidi
  libiconv
  turbojpeg-static
  libmicrohttpd.dll
  ssh
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


set(ARCH "x86-win")
set(EXECUTABLE_NAME "Plex Home Theater")
set(FFMPEG_INCLUDE_DIRS ${dependdir}/include)
set(LIBPATH .)
set(BINPATH .)
set(RESOURCEPATH .)
