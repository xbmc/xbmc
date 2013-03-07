include_directories(${CMAKE_CURRENT_SOURCE_DIR})
find_all_sources(. squish_SRCS)

if(CMAKE_SSE_CFLAGS)
  add_definitions(-DSQUISH_USE_SSE=2)
  
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_SSE_CFLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_SSE_CFLAGS}")
  
  set(CMAKE_C_FLAGS_DEBUG "-O3")
  set(CMAKE_CXX_FLAGS_DEBUG "-O3")
endif(CMAKE_SSE_CFLAGS)

add_library(lib_squish STATIC ${squish_SRCS})
