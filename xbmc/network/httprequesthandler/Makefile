SRCS=HTTPImageHandler.cpp \
     HTTPJsonRpcHandler.cpp \
     HTTPVfsHandler.cpp \
     HTTPWebinterfaceAddonsHandler.cpp \
     HTTPWebinterfaceHandler.cpp \
     IHTTPRequestHandler.cpp \

LIB=httprequesthandlers.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
