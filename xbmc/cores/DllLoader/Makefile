INCLUDES=-I. -I../../ -I../../linux -I../../../guilib -I../../../xbmc -I../../utils
CFLAGS+=-DAPI_DEBUG
CXXFLAGS+=-DAPI_DEBUG

SRCS=DllLoader.cpp DllLoaderContainer.cpp dll.cpp coff.cpp dll_tracker.cpp dll_tracker_library.cpp dll_tracker_file.cpp dll_util.cpp  SoLoader.cpp LibraryLoader.cpp ldt_keeper.c mmap_anon.c

LIB=dllloader.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
