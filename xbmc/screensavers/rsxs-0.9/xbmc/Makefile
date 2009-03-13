INCLUDES=-I. -I../ -I../linux -I../../guilib -I../utils
CFLAGS=-fPIC

OUTDIR=../../../../screensavers
RSXS=..

DIRS=$(RSXS)

RSXS-OBJS=$(RSXS)/src/common.o $(RSXS)/src/color.o $(RSXS)/lib/libmisc.a

SOLARWINDS-OUT=$(OUTDIR)/Solarwinds.xbs
SOLARWINDS-DEP=$(RSXS)/src/solarwinds/rs-solarwinds

PLASMA-OUT=$(OUTDIR)/Plasma.xbs
PLASMA-DEP=$(RSXS)/src/plasma/rs-plasma

EUPHORIA-OUT=$(OUTDIR)/Euphoria.xbs
EUPHORIA-DEP=$(RSXS)/src/euphoria/rs-euphoria

CLEAN_FILES=$(SOLARWINDS-OUT) $(PLASMA-OUT) $(EUPHORIA-OUT)

include ../../../../Makefile.include

all: $(SOLARWINDS-OUT) $(PLASMA-OUT) $(EUPHORIA-OUT);

$(SOLARWINDS-OUT): $(SOLARWINDS-DEP)
ifeq ($(findstring osx,$(ARCH)), osx)
	$(CXX) $(CFLAGS) -bundle -flat_namespace -undefined suppress -shared \
		-mmacosx-version-min=10.4 -isysroot /Developer/SDKs/MacOSX10.4u.sdk \
		-o $(SOLARWINDS-OUT) $(RSXS)/src/solarwinds/*.o $(RSXS-OBJS)
else
	$(CXX) $(CFLAGS) -shared -o $(SOLARWINDS-OUT) $(RSXS)/src/solarwinds/*.o $(RSXS-OBJS) -lXmu
endif

$(PLASMA-OUT): $(PLASMA-DEP)
ifeq ($(findstring osx,$(ARCH)), osx)
	$(CXX) $(CFLAGS) -bundle -flat_namespace -undefined suppress -shared \
		-mmacosx-version-min=10.4 -isysroot /Developer/SDKs/MacOSX10.4u.sdk \
		-o $(PLASMA-OUT)     $(RSXS)/src/plasma/*.o     $(RSXS-OBJS)
else
	$(CXX) $(CFLAGS) -shared -o $(PLASMA-OUT)     $(RSXS)/src/plasma/*.o     $(RSXS-OBJS) -lXmu
endif

$(EUPHORIA-OUT): $(EUPHORIA-DEP)
ifeq ($(findstring osx,$(ARCH)), osx)
	$(CXX) $(CFLAGS) -bundle -flat_namespace -undefined suppress -shared \
		-mmacosx-version-min=10.4 -isysroot /Developer/SDKs/MacOSX10.4u.sdk \
		-o $(EUPHORIA-OUT)   $(RSXS)/src/euphoria/*.o   $(RSXS-OBJS)
else
	$(CXX) $(CFLAGS) -shared -o $(EUPHORIA-OUT)   $(RSXS)/src/euphoria/*.o   $(RSXS-OBJS) -lXmu
endif

$(SOLARWINDS-DEP) : $(RSXS);
$(PLASMA-DEP)     : $(RSXS);
$(EUPHORIA-DEP)   : $(RSXS);

$(RSXS):
	$(MAKE) -C $(RSXS)

.PHONY: $(RSXS);
.DEFAULT_GOAL:=all

