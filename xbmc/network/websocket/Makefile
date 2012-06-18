SRCS=WebSocket.cpp \
     WebSocketManager.cpp \
     WebSocketV8.cpp \
     WebSocketV13.cpp \

LIB=websocket.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
