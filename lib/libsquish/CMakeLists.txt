
  include_directories(${CMAKE_CURRENT_SOURCE_DIR})
  find_all_sources(. squish_SRCS)
  
if (NOT TARGET_RPI)
  if(CMAKE_SSE_CFLAGS)
    add_definitions(-DSQUISH_USE_SSE=2)
    
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_SSE_CFLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_SSE_CFLAGS}")
    
    if(CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set(CMAKE_C_FLAGS_DEBUG "-O3")
      set(CMAKE_CXX_FLAGS_DEBUG "-O3")
    elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
      set(CMAKE_C_FLAGS_DEBUG "/O2")
      set(CMAKE_CXX_FLAGS_DEBUG "/O2")
    endif(CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
  
  endif(CMAKE_SSE_CFLAGS)
  
endif()
  add_library(lib_squish STATIC ${squish_SRCS})
