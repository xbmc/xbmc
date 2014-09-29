project(bspatch)
cmake_minimum_required(VERSION 2.6)
add_library(bspatch
  bspatch.c
  bspatch.h
)

if(NOT WIN32)
  add_definitions(-DBSDIFF_EXECUTABLE -DBSPATCH_EXECUTABLE)
  include_directories(${BZIP2_INCLUDE_DIR})
  add_executable(bsdiff-endsley bsdiff.c)
  add_executable(bspatch-endsley bspatch_cmd.c)
  target_link_libraries(bsdiff-endsley ${BZIP2_LIBRARY_RELEASE})
  target_link_libraries(bspatch-endsley ${BZIP2_LIBRARY_RELEASE})
endif(NOT WIN32)
