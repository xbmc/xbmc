SRCS=TestEvent.cpp

LIB=threadTest.a

CLEAN_FILES=testMain

runtest: testMain
	./testMain

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

testMain: $(LIB) ../threads.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o testMain $(LIB) ../threads.a -lboost_unit_test_framework -lboost_thread


