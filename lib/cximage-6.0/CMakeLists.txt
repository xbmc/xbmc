set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fno-common")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fno-common")
add_definitions(-DXBMC -D_DLL)

set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-alias_list ${root}/xbmc/cores/DllLoader/exports/wrapper_mach_alias")

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
add_dependencies(ImageLib-${ARCH} ${root}/xbmc/cores/DllLoader/exports/wrapper_mach_alias)

target_link_libraries(ImageLib-${ARCH} ${CONFIG_LIBRARY_Z} ${CONFIG_LIBRARY_PNG} ${CONFIG_LIBRARY_JPEG} ${CONFIG_LIBRARY_TIFF})

install(TARGETS ImageLib-${ARCH} DESTINATION ${CMAKE_INSTALL_PREFIX}/Plex.app/Contents/Resources/XBMC/system)
