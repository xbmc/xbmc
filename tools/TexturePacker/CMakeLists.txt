set(SRC md5.cpp SDL_anigif.cpp XBTFWriter.cpp XBMCTex.cpp ${root}/xbmc/guilib/XBTF.cpp)
include_directories(${root}/lib ${root}/xbmc ${CMAKE_BINARY_DIR}/xbmc ${root}/xbmc/linux)
add_definitions(-DUSE_LZO_PACKING)
if(APPLE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
endif()
add_executable(TexturePacker ${SRC})
target_link_libraries(TexturePacker
  ${CONFIG_LIBRARY_SDL_IMAGE}
  ${CONFIG_LIBRARY_SDL}
  ${CONFIG_LIBRARY_LZO2}
  ${CONFIG_LIBRARY_STDC++}
  lib_squish)