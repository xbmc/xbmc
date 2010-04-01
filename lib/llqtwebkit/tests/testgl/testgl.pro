TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
INCLUDEPATH += ../../
CONFIG -= app_bundle

QT += webkit opengl network

!mac {
unix {
    DEFINES += LL_LINUX
#    DEFINES += LL_LINUX LL_NEWER_GLUI
    LIBS += -lglui -lglut
    LIBS += $$PWD/../../libllqtwebkit.a
}
}

mac {
    DEFINES += LL_OSX
    LIBS += -framework GLUT -framework OpenGL
   LIBS += $$PWD/libllqtwebkit.dylib
}


win32{
    DEFINES += _WINDOWS
    INCLUDEPATH += ../
    LIBS += -L../GL 
    DESTDIR=../GL
    debug {
      LIBS += $$PWD/../../Debug/llqtwebkitd.lib
    }
    release {
      LIBS += $$PWD/../../Release/llqtwebkit.lib
    }
}

include(../../static.pri)

# Input
SOURCES += testgl.cpp
