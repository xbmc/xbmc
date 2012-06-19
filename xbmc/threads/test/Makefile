SRCS=	\
	TestMain.cpp \
	TestEvent.cpp \
	TestSharedSection.cpp \
	TestAtomics.cpp \
	TestThreadLocal.cpp


LIB=threadTest.a

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

testMain: $(LIB) ../threads.a $(GTEST_LIBS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(GTEST_INCLUDES) -o testMain $(OBJS) ../threads.a ../../commons/commons.a $(GTEST_LIBS) -lpthread -lrt


