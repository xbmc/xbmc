INCLUDES=-I. -I../ -I../linux -I../../guilib -I../utils

SRCS=fft.cpp Visualisation.cpp VisualisationFactory.cpp

LIB=visualizations.a
DIRS=OpenGLSpectrum WaveForm

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

lib: $(OBJS)
	for DIR in $(DIRS); do make -C $$DIR; done
