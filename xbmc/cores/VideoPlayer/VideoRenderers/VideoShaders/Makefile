SRCS=YUV2RGBShader.cpp VideoFilterShader.cpp ConvolutionKernels.cpp GLSLOutput.cpp

LIB=VideoShaders.a

include ../../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
