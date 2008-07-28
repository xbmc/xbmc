INCLUDES=-I. -I../ -I../linux -I../../guilib -I../utils

OUTDIR=../../screensavers
RSXS=rsxs-0.9

SRCS=ScreenSaver.cpp ScreenSaverFactory.cpp
DIRS=$(RSXS) $(RSXS)/src/plasma $(RSXS)/src/solarwinds

RSXS-OBJS=$(RSXS)/src/common.o $(RSXS)/src/color.o $(RSXS)/lib/libmisc.a

SOLARWINDS-OUT=$(OUTDIR)/Solarwinds.xbs
SOLARWINDS-DEP=$(RSXS)/src/solarwinds/rs-solarwinds

PLASMA-OUT=$(OUTDIR)/Plasma.xbs
PLASMA-DEP=$(RSXS)/src/plasma/rs-plasma

include ../../Makefile.include
-include $(OBJS:.o=.P)

all: $(OBJS) $(SOLARWINDS-OUT) $(PLASMA-OUT);

$(SOLARWINDS-OUT): $(SOLARWINDS-DEP)
	g++ -shared -o $(SOLARWINDS-OUT) $(RSXS)/src/solarwinds/*.o $(RSXS-OBJS) -lXmu

$(PLASMA-OUT): $(PLASMA-DEP)
	g++ -shared -o $(PLASMA-OUT)     $(RSXS)/src/plasma/*.o     $(RSXS-OBJS) -lXmu

$(SOLARWINDS-DEP) : $(RSXS);
$(PLASMA-DEP)     : $(RSXS);

$(RSXS):
	$(MAKE) -C $(RSXS)

.PHONY: $(RSXS);
.DEFAULT_GOAL:=all

