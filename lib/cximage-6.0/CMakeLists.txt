set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fno-common")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fno-common")
add_definitions(-DXBMC -D_DLL)

add_subdirectory(CxImage)
add_subdirectory(jbig)
add_subdirectory(raw)
add_subdirectory(jasper)

add_library(ImageLib-${ARCH} MODULE 
  $<TARGET_OBJECTS:wrapper>
  $<TARGET_OBJECTS:cximage>
  $<TARGET_OBJECTS:cx_jbig>
  $<TARGET_OBJECTS:cx_raw>
  $<TARGET_OBJECTS:cx_jasper>
)

target_link_libraries(ImageLib-${ARCH} ${z_LIBRARY} ${png_LIBRARY} ${jpeg_LIBRARY} ${tiff_LIBRARY})

install(TARGETS ImageLib-${ARCH} DESTINATION ${CMAKE_INSTALL_PREFIX}/Plex.app/Contents/Resources/XBMC/system)
