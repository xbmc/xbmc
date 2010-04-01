TEMPLATE = lib
CONFIG += static staticlib # we always build as static lib whether Qt is static or not
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

include(llqtwebkit.pri)

QT += webkit opengl network gui

win32:CONFIG(debug,debug|release) {
  TARGET = llqtwebkitd
}

RCC_DIR     = $$PWD/.rcc
UI_DIR      = $$PWD/.ui
MOC_DIR     = $$PWD/.moc
OBJECTS_DIR = $$PWD/.obj
