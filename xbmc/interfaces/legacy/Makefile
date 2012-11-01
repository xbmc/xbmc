INCLUDES=-I../../.. -I. -I../../ -I../../linux -I../../../guilib -I.
SRCS=	\
	Addon.cpp \
	AddonCallback.cpp \
	AddonClass.cpp \
	AddonUtils.cpp \
	CallbackFunction.cpp \
	CallbackHandler.cpp \
	Control.cpp \
	Dialog.cpp \
	File.cpp \
	InfoTagMusic.cpp \
	InfoTagVideo.cpp \
	Keyboard.cpp \
	LanguageHook.cpp \
	ListItem.cpp \
	ModuleXbmc.cpp \
	ModuleXbmcgui.cpp \
	ModuleXbmcplugin.cpp \
	ModuleXbmcvfs.cpp \
	Monitor.cpp \
	Player.cpp \
	PlayList.cpp \
	String.cpp \
	Window.cpp \
	WindowDialog.cpp \
	WindowDialogMixin.cpp \
	WindowXML.cpp \

LIB=legacy.a

CXXFLAGS+= -DSWIGRUNTIME_DEBUG -DSTATIC_LINKED

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

