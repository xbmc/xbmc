project(crossguid)

cmake_minimum_required(VERSION 3.0)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

find_package(CXX11 REQUIRED)

add_library(${PROJECT_NAME} guid.cpp)

if(UNIX AND NOT APPLE)
  find_package(UUID REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE UUID::UUID)
  target_compile_definitions(${PROJECT_NAME} PRIVATE GUID_LIBUUID)
elseif(APPLE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE GUID_CFUUID)
elseif(WIN32)
  target_compile_definitions(${PROJECT_NAME} PRIVATE GUID_WINDOWS)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(FILES guid.h DESTINATION include)
