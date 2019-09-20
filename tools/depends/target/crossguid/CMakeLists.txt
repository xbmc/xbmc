cmake_minimum_required(VERSION 3.1)
project(crossguid)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

add_library(${PROJECT_NAME} guid.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 11
                                                 CXX_STANDARD_REQUIRED True
                                                 CXX_EXTENSIONS False)

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
