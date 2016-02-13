if(CMAKE_CROSSCOMPILING)
  return()
endif()

set(SOURCES src/alpha.cpp
            src/clusterfit.cpp
            src/colourblock.cpp
            src/colourfit.cpp
            src/colourset.cpp
            src/maths.cpp
            src/rangefit.cpp
            src/singlecolourfit.cpp
            src/squish.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

find_package(SSE)
if(SSE2_FOUND)
  add_definitions(-DSQUISH_USE_SSE=2)
elseif(SSE_FOUND)
  add_definitions(-DSQUISH_USE_SSE=1)
endif()

# Always compile release - it's insanely slow in debug
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
add_definitions(-DNDEBUG)

if(SQUISH_NATIVE)
  core_add_library(squish-native NO_MAIN_DEPENDS)
else()
  if(NOT BOOTSTRAP_IN_TREE)
    core_add_library(squish)
  else()
    set(core_DEPENDS squish-native ${core_DEPENDS} CACHE STRING "" FORCE)
  endif()
endif()
