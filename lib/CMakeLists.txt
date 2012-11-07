add_subdirectory(libsquish)
add_subdirectory(libhts)

# build our external projects
set(CMAKE_SHARED_MODULE_PREFIX "")
set(CMAKE_SHARED_MODULE_SUFFIX ".so")
if(APPLE)
	set(CMAKE_MODULE_LINKER_FLAGS "-arch i386 -undefined dynamic_lookup -read_only_relocs suppress")
	set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "-bundle")
elseif(UNIX)
    set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--unresolved-symbols=ignore-all -Wl,-wrap,_IO_getc -Wl,-wrap,_IO_getc_unlocked -Wl,-wrap,_IO_putc -Wl,-wrap,__fgets_chk -Wl,-wrap,__fprintf_chk -Wl,-wrap,__fread_chk -Wl,-wrap,__fxstat64 -Wl,-wrap,__lxstat64 -Wl,-wrap,__printf_chk -Wl,-wrap,__read_chk -Wl,-wrap,__vfprintf_chk -Wl,-wrap,__xstat64 -Wl,-wrap,_stat -Wl,-wrap,calloc -Wl,-wrap,clearerr -Wl,-wrap,close -Wl,-wrap,closedir -Wl,-wrap,dlopen -Wl,-wrap,fclose -Wl,-wrap,fdopen -Wl,-wrap,feof -Wl,-wrap,ferror -Wl,-wrap,fflush -Wl,-wrap,fgetc -Wl,-wrap,fgetpos -Wl,-wrap,fgetpos64 -Wl,-wrap,fgets -Wl,-wrap,fileno -Wl,-wrap,flockfile -Wl,-wrap,fopen -Wl,-wrap,fopen64 -Wl,-wrap,fprintf -Wl,-wrap,fputc -Wl,-wrap,fputs -Wl,-wrap,fread -Wl,-wrap,free -Wl,-wrap,freopen -Wl,-wrap,fseek -Wl,-wrap,fseeko64 -Wl,-wrap,fsetpos -Wl,-wrap,fsetpos64 -Wl,-wrap,fstat -Wl,-wrap,ftell -Wl,-wrap,ftello64 -Wl,-wrap,ftrylockfile -Wl,-wrap,funlockfile -Wl,-wrap,fwrite -Wl,-wrap,getc -Wl,-wrap,getc_unlocked -Wl,-wrap,getmntent -Wl,-wrap,ioctl -Wl,-wrap,lseek -Wl,-wrap,lseek64 -Wl,-wrap,malloc -Wl,-wrap,open -Wl,-wrap,open64 -Wl,-wrap,opendir -Wl,-wrap,popen -Wl,-wrap,printf -Wl,-wrap,read -Wl,-wrap,readdir -Wl,-wrap,readdir64 -Wl,-wrap,realloc -Wl,-wrap,rewind -Wl,-wrap,rewinddir -Wl,-wrap,setvbuf -Wl,-wrap,ungetc -Wl,-wrap,vfprintf -Wl,-wrap,write")
endif()


add_subdirectory(cpluff)
add_subdirectory(cximage-6.0)
