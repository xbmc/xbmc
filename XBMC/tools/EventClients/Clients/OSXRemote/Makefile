TOP=../../../../
TARGET = ../../../osx/XBMCHelper

all: $(TARGET)

include $(TOP)Makefile.include

#check for debug build
ifneq (,$(findstring _DEBUG, $(CXXFLAGS)))
  CONFIGURATION=Debug
else
  CONFIGURATION=Release
endif

$(TARGET): 
	echo $(CXXFLAGS)
	xcodebuild -sdk macosx10.4 -configuration $(CONFIGURATION)

clean:
	xcodebuild clean -configuration $(CONFIGURATION)

.PHONY: all clean
