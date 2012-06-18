SRCS=	\
	TestMain.cpp \
	TestGlobalsHandling.cpp

LIB=utilsTest.a

CLEAN_FILES=testMain

check: testMain
	./testMain

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

testMain: $(LIB)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o testMain -Wl,--whole-archive $(LIB) -Wl,--no-whole-archive -lboost_unit_test_framework


