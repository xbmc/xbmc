INCLUDES=-I. -I../ -I../linux -I../../guilib -I../utils

SRCS=fft.cpp Visualisation.cpp VisualisationFactory.cpp
DIRS=OpenGLSpectrum WaveForm XBMCProjectM

include ../../Makefile.include
-include $(OBJS:.o=.P)

lib: $(DIRS) $(OBJS)

.PHONY: $(DIRS)
$(DIRS):
	$(MAKE) -C $@

