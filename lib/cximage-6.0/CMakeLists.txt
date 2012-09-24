add_subdirectory(CxImage)
add_subdirectory(jbig)
add_subdirectory(raw)
add_subdirectory(jasper)
add_subdirectory(jpeg)
add_subdirectory(tiff)
add_subdirectory(png)

set(CMAKE_SHARED_MODULE_PREFIX "")
set(CMAKE_SHARED_MODULE_SUFFIX ".so")
if(APPLE)
	set(CMAKE_MODULE_LINKER_FLAGS "-arch i386 -undefined dynamic_lookup -read_only_relocs suppress -Wl,-alias_list ${root}/xbmc/cores/DllLoader/exports/wrapper_mach_alias")
	set(ARCH x86-osx)
endif()

add_library(ImageLib-${ARCH} MODULE 
	$<TARGET_OBJECTS:cximage>
	$<TARGET_OBJECTS:cx_jbig>
	$<TARGET_OBJECTS:cx_raw>
	$<TARGET_OBJECTS:cx_jasper>
	$<TARGET_OBJECTS:cx_jpeg>
	$<TARGET_OBJECTS:cx_tiff>
  $<TARGET_OBJECTS:cx_png>
	$<TARGET_OBJECTS:wrapper>
)

target_link_libraries(ImageLib-${ARCH} ${z_LIBRARY})

install(TARGETS ImageLib-${ARCH} DESTINATION ${CMAKE_INSTALL_PREFIX}/Plex.app/Contents/Resources/XBMC/system)
