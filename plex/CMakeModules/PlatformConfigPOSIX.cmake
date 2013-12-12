######### Shared configuration between POSIX platforms
############ Special Clang cflags
option(CLANG_COLOR "Show CLang color during compile" ON)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
  if(CLANG_COLOR)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -fcolor-diagnostics")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -fcolor-diagnostics")
  endif(CLANG_COLOR)
endif(CMAKE_C_COMPILER_ID STREQUAL "Clang")

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -Wno-parentheses-equality -Wno-self-assign-field")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -Wno-parentheses-equality -Wno-self-assign-field")
endif(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")  

############ Set global CFlags with the information from the subroutines
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS}")

############ Disable optimization when building for Debug
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

############ Generate debug symbols even when we build for Release
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g -Os")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -Os")

############ Check for pthread_setname -> HAVE_PTHREAD_SETNAME_NP
set(HAVE_PTHREAD_SETNAME_NP 1)
set(HAS_SDL_JOYSTICK 1)
set(HAS_LIBRTMP 1)

if(NOT USE_INTERNAL_FFMPEG)
  add_definitions(-DUSE_EXTERNAL_FFMPEG)
else()
  add_definitions(-DUSE_INTERNAL_FFMPEG)
endif()

add_definitions(
  -DTARGET_POSIX
  -D_REENTRANT
  -D_FILE_DEFINED
  -D__STDC_CONSTANT_MACROS
  -DHAVE_CONFIG_H
  -D_FILE_OFFSET_BITS=64
  -D_LARGEFILE64_SOURCE
  -D_LINUX
  -D__STDC_LIMIT_MACROS
)
