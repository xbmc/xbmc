SRCS=	\
	TestMain.cpp \
	TestGlobalsHandling.cpp

LIB=utilsTest.a

CLEAN_FILES=testMain

runtest: testMain
	./testMain

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

testMain: $(LIB)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o testMain $(OBJS) -lboost_unit_test_framework


