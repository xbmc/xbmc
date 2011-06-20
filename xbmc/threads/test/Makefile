SRCS=TestEvent.cpp

LIB=threadTest.a

bin : threadTest .PHONY

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

threadTest: $(LIB)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o threadTest $(LIB) ../threads.a ../../utils/utils.a -lboost_unit_test_framework -lboost_thread


