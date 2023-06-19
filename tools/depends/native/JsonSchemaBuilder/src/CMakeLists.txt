cmake_minimum_required(VERSION 3.15)
project(JsonSchemaBuilder)

set(SOURCES JsonSchemaBuilder.cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(JsonSchemaBuilder ${SOURCES})

install(TARGETS JsonSchemaBuilder DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        RENAME "${APP_NAME_LC}-JsonSchemaBuilder")
