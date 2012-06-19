SRCS=	\
	TestMain.cpp \
	TestGlobalsHandling.cpp

LIB=utilsTest.a

CLEAN_FILES=testMain

GTEST_INCLUDES = -I../../../lib/gtest/include
GTEST_LIBS = ../../../lib/gtest/lib/.libs/libgtest.a \
             ../../../lib/gtest/lib/.libs/libgtest_main.a

INCLUDES += $(GTEST_INCLUDES)

check: testsuite
	./testMain

testsuite: testMain

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

testMain: $(LIB) $(GTEST_LIBS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(GTEST_INCLUDES) -o testMain -Wl,--whole-archive $(LIB) -Wl,--no-whole-archive $(GTEST_LIBS) -lpthread
