project(tinyxml)

cmake_minimum_required(VERSION 2.6)

set(SOURCES
  tinyxml2.cpp
)

set(HEADERS
  tinyxml2.h
)

add_library(tinyxml
  ${SOURCES}
  ${HEADERS}
)

