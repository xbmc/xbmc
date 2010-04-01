TEMPLATE = app
TARGET =
DEPENDPATH += .
INCLUDEPATH += .

win32: CONFIG += console
mac:CONFIG -= app_bundle

CONFIG += qtestlib

include(../../src/src.pri)
#include(../../../dev/code/webweaver/src/iris.pri)
#include(../../../dev/arora/src/src.pri)

# Input
SOURCES += main.cpp
HEADERS +=
