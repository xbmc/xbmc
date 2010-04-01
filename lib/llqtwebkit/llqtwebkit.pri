DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD

!mac {
unix {
    DEFINES += LL_LINUX
}
}

mac {
    DEFINES += LL_OSX
}

win32{
    DEFINES += _WINDOWS
}

# Input
HEADERS += llembeddedbrowser.h \
           llembeddedbrowser_p.h \
           llembeddedbrowserwindow.h \
           llembeddedbrowserwindow_p.h \
           llnetworkaccessmanager.h \
           llqtwebkit.h \
           llwebpage.h \
 	       llstyle.h

SOURCES += llembeddedbrowser.cpp \
           llembeddedbrowserwindow.cpp \
           llnetworkaccessmanager.cpp \
           llqtwebkit.cpp \
           llwebpage.cpp \
	       llstyle.cpp

FORMS +=   passworddialog.ui

RCC_DIR     = .rcc
UI_DIR      = .ui
MOC_DIR     = .moc
OBJECTS_DIR = .obj

include(static.pri)
include(qtwebkit_cookiejar/src/src.pri)
