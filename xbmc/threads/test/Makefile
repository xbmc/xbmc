SRCS=	\
	TestMain.cpp \
	TestEvent.cpp \
	TestSharedSection.cpp \
	TestAtomics.cpp \
	TestThreadLocal.cpp


LIB=threadTest.a

CLEAN_FILES=testMain

check: testMain
	./testMain

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

testMain: $(LIB) ../threads.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o testMain $(OBJS) ../threads.a ../../commons/commons.a -lunittest++ -lpthread -lrt


