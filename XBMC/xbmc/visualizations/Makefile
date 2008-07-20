INCLUDES=-I. -I../ -I../linux -I../../guilib -I../utils

SRCS=fft.cpp Visualisation.cpp VisualisationFactory.cpp

LIB=visualizations.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

lib:
	$(MAKE) -C OpenGLSpectrum vis
