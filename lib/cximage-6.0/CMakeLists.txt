if(NOT TARGET_WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fno-common")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fno-common")
  add_definitions(-D_DLL -DXBMC)
  include_directories(${dependdir}/include)
else(NOT TARGET_WIN32)
  set(COMPILE_DEFINITIONS "")
  add_definitions(-D_DLL -DWIN32 -D_WINDOWS -D_USRDLL -D_AFXDLL -DJAS_WIN_MSVC_BUILD -D_CRT_SECURE_NO_DEPRECATE -D_CONSOLE)
  set(CMAKE_C_FLAGS_DEBUG "/MP /Od /Oy- /D_DEBUG /Gm- /MDd /GS /arch:SSE /fp:precise /Zc:wchar_t /Zc:forScope /wd\"4996\"")
  set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
  set(CMAKE_C_FLAGS_RELEASE " /WX- /O2 /Ob1 /Oy- /DNDEBUG /GF /Gm- /EHa /MD /GS- /Gy /arch:SSE /fp:precise /Zc:wchar_t /Zc:forScope")
  set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
endif(NOT TARGET_WIN32)

set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PLEX_LINK_WRAPPED}")

#        $(CXX) $(CFLAGS) $(LDFLAGS) -shared -o $(SLIB) -Wl,--whole-archive $(SLIBS) \
#                -Wl,--no-whole-archive -lpng -ljasper -ljpeg -ltiff -lz -Wl,--unresolved-symbols=ignore-all \
#                                `cat ../../xbmc/cores/DllLoader/exports/wrapper.def` \
#                                                ../../xbmc/cores/DllLoader/exports/wrapper.o

if(TARGET_WIN32)
  # Use the zlib that comes with cximage
  include_directories(zlib zlib/win32)
endif(TARGET_WIN32)

add_subdirectory(CxImage)
add_subdirectory(jbig)
add_subdirectory(raw)
add_subdirectory(jasper)
add_subdirectory(zlib)

set(OBJECTS
  cximage
  cx_jbig
  cx_raw
  cx_jasper
)
set(SRC CxImage/DllInterface.cpp)

if(NOT TARGET_WIN32)
  list(APPEND OBJECTS wrapper)
  add_library(ImageLib-${ARCH} MODULE ${SRC})
  target_link_libraries(ImageLib-${ARCH} cx_zlib ${PLEX_LINK_WHOLEARCHIVE} ${OBJECTS} ${PLEX_LINK_NOWHOLEARCHIVE} ${CONFIG_LIBRARY_PNG} ${CONFIG_LIBRARY_JPEG} ${CONFIG_LIBRARY_TIFF})
else(NOT TARGET_WIN32)
  add_subdirectory(zlib)
  add_subdirectory(jpeg)
  add_subdirectory(mng)
  add_subdirectory(tiff)
  add_subdirectory(png)

  set(OBJECTS ${OBJECTS}
    cx_mng
    cx_jpeg
    cx_png
    cx_tiff
    cx_zlib
  )

  add_library(ImageLib SHARED ${SRC})
  target_link_libraries(ImageLib ${OBJECTS})

  set(LIBS kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32 ole32 oleaut32 uuid odbc32 odbccp32)
  foreach(l ${LIBS})
    find_library(t ${l})
    target_link_libraries(ImageLib ${t})
  endforeach(l ${LIBS})
endif(NOT TARGET_WIN32)

if(TARGET_COMMON_DARWIN)
  install(TARGETS ImageLib-${ARCH} DESTINATION ${RESOURCEPATH}/system COMPONENT RUNTIME)
elseif(TARGET_COMMON_LINUX)
  install(TARGETS ImageLib-${ARCH} DESTINATION ${BINPATH}/system COMPONENT RUNTIME)
elseif(TARGET_WIN32)
  install(TARGETS ImageLib RUNTIME DESTINATION ${RESOURCEPATH}/system LIBRARY DESTINATION ${RESOURCEPATH}/system COMPONENT RUNTIME)
endif(TARGET_COMMON_DARWIN)
