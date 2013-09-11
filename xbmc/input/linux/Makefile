SRCS=LIRC.cpp \
     LinuxInputDevices.cpp \
     LinuxJoystick.cpp \
     LinuxJoystickSDL.cpp

LIB=input_linux.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
