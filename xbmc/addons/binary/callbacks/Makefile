INCLUDES+=-I/home/alwin/Development/3/kodi/addons/library.kodi.addon \
          -I/home/alwin/Development/3/kodi/addons/library.kodi.audioengine \
          -I/home/alwin/Development/3/kodi/addons/library.kodi.guilib \
          -I/home/alwin/Development/3/kodi/addons/library.kodi.pvr \

SRCS=AddonCallbacks.cpp \
     AddonCallbacksAddonBase.cpp \

LIB=addon-callbacks.a

include /home/alwin/Development/3/kodi/Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
