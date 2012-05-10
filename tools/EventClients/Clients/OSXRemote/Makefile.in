#convenience makefile to allow headless osx build

DEBUG_FLAGS=@DEBUG_FLAGS@

#check for debug build
ifneq (,$(findstring _DEBUG, $(DEBUG_FLAGS)))
  CONFIGURATION=Debug
else
  CONFIGURATION=Release
endif

all: 
	xcodebuild -configuration $(CONFIGURATION)

clean:
	xcodebuild clean -configuration $(CONFIGURATION)

.PHONY: all clean
