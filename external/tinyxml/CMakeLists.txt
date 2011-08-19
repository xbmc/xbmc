project(tinyxml)

cmake_minimum_required(VERSION 2.6)

Add_definitions(-DTIXML_USE_STL)

set(SOURCES
  tinystr.cpp
  tinyxml.cpp
  tinyxmlerror.cpp
  tinyxmlparser.cpp
)

set(HEADERS
  tinystr.h
  tinyxml.h
)

add_library(tinyxml
  ${SOURCES}
  ${HEADERS}
)

