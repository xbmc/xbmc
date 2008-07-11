INCLUDES=-I. -I../../ -I../../linux -I../../../guilib -I../../../xbmc -I../../utils
CFLAGS+=-DAPI_DEBUG

SRCS=DllLoader.cpp DllLoaderContainer.cpp dll.cpp coff.cpp dll_tracker.cpp dll_tracker_library.cpp dll_tracker_file.cpp dll_tracker_memory.cpp dll_tracker_critical_section.cpp dll_util.cpp  SoLoader.cpp LibraryLoader.cpp

# this includes on the .o of the C files
OBJS=ldt_keeper.o mmap_anon.o

LIB=dllloader.a

include ../../../Makefile.include
-include Makefile.depend
