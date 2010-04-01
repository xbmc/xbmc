TEMPLATE = app
TARGET =
DEPENDPATH += .
INCLUDEPATH += .

win32: CONFIG += console
mac:CONFIG -= app_bundle

CONFIG += qtestlib

include(../../src/src.pri)

# Input
SOURCES += tst_trie.cpp
HEADERS +=
