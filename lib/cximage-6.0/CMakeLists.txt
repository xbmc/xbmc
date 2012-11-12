set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fno-common")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fno-common")
add_definitions(-DXBMC -D_DLL)

if(APPLE)
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-alias_list ${root}/xbmc/cores/DllLoader/exports/wrapper_mach_alias")
elseif(UNIX)
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--unresolved-symbols=ignore-all -Wl,-wrap,_IO_getc -Wl,-wrap,_IO_getc_unlocked -Wl,-wrap,_IO_putc -Wl,-wrap,__fgets_chk -Wl,-wrap,__fprintf_chk -Wl,-wrap,__fread_chk -Wl,-wrap,__fxstat64 -Wl,-wrap,__lxstat64 -Wl,-wrap,__printf_chk -Wl,-wrap,__read_chk -Wl,-wrap,__vfprintf_chk -Wl,-wrap,__xstat64 -Wl,-wrap,_stat -Wl,-wrap,calloc -Wl,-wrap,clearerr -Wl,-wrap,close -Wl,-wrap,closedir -Wl,-wrap,dlopen -Wl,-wrap,fclose -Wl,-wrap,fdopen -Wl,-wrap,feof -Wl,-wrap,ferror -Wl,-wrap,fflush -Wl,-wrap,fgetc -Wl,-wrap,fgetpos -Wl,-wrap,fgetpos64 -Wl,-wrap,fgets -Wl,-wrap,fileno -Wl,-wrap,flockfile -Wl,-wrap,fopen -Wl,-wrap,fopen64 -Wl,-wrap,fprintf -Wl,-wrap,fputc -Wl,-wrap,fputs -Wl,-wrap,fread -Wl,-wrap,free -Wl,-wrap,freopen -Wl,-wrap,fseek -Wl,-wrap,fseeko64 -Wl,-wrap,fsetpos -Wl,-wrap,fsetpos64 -Wl,-wrap,fstat -Wl,-wrap,ftell -Wl,-wrap,ftello64 -Wl,-wrap,ftrylockfile -Wl,-wrap,funlockfile -Wl,-wrap,fwrite -Wl,-wrap,getc -Wl,-wrap,getc_unlocked -Wl,-wrap,getmntent -Wl,-wrap,ioctl -Wl,-wrap,lseek -Wl,-wrap,lseek64 -Wl,-wrap,malloc -Wl,-wrap,open -Wl,-wrap,open64 -Wl,-wrap,opendir -Wl,-wrap,popen -Wl,-wrap,printf -Wl,-wrap,read -Wl,-wrap,readdir -Wl,-wrap,readdir64 -Wl,-wrap,realloc -Wl,-wrap,rewind -Wl,-wrap,rewinddir -Wl,-wrap,setvbuf -Wl,-wrap,ungetc -Wl,-wrap,vfprintf -Wl,-wrap,write")
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

if(UNIX)
  set(LIBS ${CONFIG_LIBRARY_Z} ${CONFIG_LIBRARY_PNG} ${CONFIG_LIBRARY_JPEG} ${CONFIG_LIBRARY_TIFF})
else()
  set(LIBS ${CONFIG_LIBRARY_ZLIB} ${CONFIG_LIBRARY_TURBOJPEG-STATIC})
endif()

target_link_libraries(ImageLib-${ARCH} ${LIBS})
if(APPLE)
  install(TARGETS ImageLib-${ARCH} DESTINATION ${RESOURCEPATH}/XBMC/system)
elseif(UNIX)
  install(TARGETS ImageLib-${ARCH} DESTINATION ${BINPATH}/system)
endif()
