APP=JsonSchemaBuilder

SOURCES := $(shell ls *.cpp)

OBJS := $(SOURCES:.cpp=.o)
all: $(APP)

$(APP): $(OBJS)
	$(CXX) $(CFLAGS) -o $(APP) $(OBJS) $(LFLAGS) $(LIBS)

.cpp.o:
	$(CXX) $(CFLAGS) $(INCLUDES) -c $<

clean: 
	rm -f $(OBJS) $(SOURCES:.cpp=.d) $(APP)
