SRCS= \
  TestDirectory.cpp \
  TestFile.cpp \
  TestFileFactory.cpp \
  TestRarFile.cpp \
  TestZipFile.cpp

LIB=filesystemTest.a

INCLUDES += -I../../../lib/gtest/include

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
