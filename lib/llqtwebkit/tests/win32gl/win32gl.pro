TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += . ../..

QT += webkit network opengl

# Input
SOURCES += win32gl.cpp
DEFINES -= UNICODE

debug {
    LIBS += $$PWD/../../Debug/llqtwebkitd.lib
}
release {
    LIBS += $$PWD/../../Release/llqtwebkit.lib
}
