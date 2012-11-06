set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fno-common")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fno-common")
add_definitions(-DXBMC -D_DLL)

if(APPLE)
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-alias_list ${root}/xbmc/cores/DllLoader/exports/wrapper_mach_alias")
elseif(UNIX)
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--unresolved-symbols=ignore-all ")
endif()

#        $(CXX) $(CFLAGS) $(LDFLAGS) -shared -o $(SLIB) -Wl,--whole-archive $(SLIBS) \
#                -Wl,--no-whole-archive -lpng -ljasper -ljpeg -ltiff -lz -Wl,--unresolved-symbols=ignore-all \
#                                `cat ../../xbmc/cores/DllLoader/exports/wrapper.def` \
#                                                ../../xbmc/cores/DllLoader/exports/wrapper.o

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

target_link_libraries(ImageLib-${ARCH} ${CONFIG_LIBRARY_Z} ${CONFIG_LIBRARY_PNG} ${CONFIG_LIBRARY_JPEG} ${CONFIG_LIBRARY_TIFF})
if(APPLE)
  install(TARGETS ImageLib-${ARCH} DESTINATION ${RESOURCEPATH}/XBMC/system)
elseif(UNIX)
  install(TARGETS ImageLib-${ARCH} DESTINATION ${BINPATH}/system)
endif()
