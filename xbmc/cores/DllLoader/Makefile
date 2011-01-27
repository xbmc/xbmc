CFLAGS+=-DAPI_DEBUG
CXXFLAGS+=-DAPI_DEBUG

SRCS=coff.cpp \
     dll.cpp \
     DllLoader.cpp \
     DllLoaderContainer.cpp \
     dll_tracker.cpp \
     dll_tracker_file.cpp \
     dll_tracker_library.cpp \
     dll_util.cpp \
     ldt_keeper.c \
     LibraryLoader.cpp \
     mmap_anon.c \
     SoLoader.cpp \

LIB=dllloader.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
