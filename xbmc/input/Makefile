SRCS=ButtonTranslator.cpp \
     InertialScrollingHandler.cpp \
     KeyboardLayout.cpp \
     KeyboardLayoutManager.cpp \
     KeyboardStat.cpp \
     MouseStat.cpp \
     SDLJoystick.cpp \
     XBMC_keytable.cpp \
     InputManager.cpp \
     Key.cpp
     
LIB=input.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
