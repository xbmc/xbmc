INCLUDES=-I../../../.. -I. -I../../../ -I../../../linux -I../../../../guilib -I../../../utils -I../../../cores -I../../../../lib/jsoncpp/jsoncpp/include

SRCS=action.cpp controlbutton.cpp controlcheckmark.cpp control.cpp controlfadelabel.cpp controlimage.cpp controllabel.cpp controllist.cpp controlprogress.cpp controlslider.cpp controlspin.cpp controltextbox.cpp dialog.cpp GUIPythonWindow.cpp GUIPythonWindowDialog.cpp GUIPythonWindowXML.cpp GUIPythonWindowXMLDialog.cpp infotagmusic.cpp infotagvideo.cpp keyboard.cpp listitem.cpp player.cpp pyplaylist.cpp PythonPlayer.cpp pyutil.cpp window.cpp winxml.cpp winxmldialog.cpp xbmcguimodule.cpp xbmcmodule.cpp controlgroup.cpp xbmcplugin.cpp controlradiobutton.cpp PythonAddon.cpp xbmcaddonmodule.cpp

LIB=xbmcmodule.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
