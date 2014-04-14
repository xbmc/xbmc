set(dependdir ${root}/project/BuildDependencies)

######################### Linker flags
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "c:/Windows/System32" "${dependdir}/lib" CACHE STRING "")
link_directories(${dependdir}/lib)
set(ENV{LIBS} "$ENV{LIBS};${dependdir}/lib")

######################### Compiler CFLAGS
# C4800 = 'unsigned int' : forcing value to bool 'true' or 'false' (performance warning)
# C4996 = 'strcmpi': The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _strcmpi. See online help for details
# C4244 = 'initializing' : conversion from 'int64_t' to 'int', possible loss of data
set(IGNOREERRS "/wd4800 /wd4996 /wd4244 /wd4804")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MDd ${IGNOREERRS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd ${IGNOREERRS}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MD ${IGNOREERRS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD ${IGNOREERRS}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MD ${IGNOREERRS}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MD ${IGNOREERRS}")

set(IGNORELIBS
libc.lib
libcmt.lib
msvcrt.lib
libcd.lib
)

set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib")
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib")

find_package(DirectX REQUIRED)
include_directories(${DirectX_INCLUDE_DIR})

set(system_libs
  yajl
  dwmapi
  winmm
  Mpr
  Iphlpapi
  PowrProf
  setupapi
  Delayimp
  ${DirectX_LIBRARIES}
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

set(external_libs ${external_libs})

foreach(lib ${external_libs})
  plex_find_library(${lib} 0 1 ${dependdir}/lib 1)
endforeach()

foreach(lib ${non_link_libs})
  plex_find_library(${lib} 0 1 ${dependdir}/lib 0)
endforeach()

foreach(lib ${system_libs})
  list(APPEND CONFIG_PLEX_LINK_LIBRARIES ${lib})
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
  -DNOMINMAX
  -D_USE_32BIT_TIME_T
  -DHAS_DX
  -DD3D_DEBUG_INFO
  -D__STDC_CONSTANT_MACROS
  -DTAGLIB_STATIC
  -DNTDDI_VERSION=0x05010300
  -D_WIN32_WINNT=0x0501
  -DBOOST_NO_0X_HDR_INITIALIZER_LIST
  -D_VARIADIC_MAX=10
)

