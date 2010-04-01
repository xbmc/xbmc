#!/bin/bash

CXX=g++
INCFLAGS=-I../..
CXXOPT="-ggdb -DLL_NEWER_GLUI -DLL_LINUX `pkg-config gtk+-2.0 --cflags` -L$QTDIR/lib -L../.. -L../../libraries/`arch -m`-linux/lib_release"
#LIBFLAGS="-lllmozlib2 -lmozjs -lnspr4 -lplc4 -lplds4 -lprofdirserviceprovider_s -lxul -lglui `pkg-config gtk+-2.0 --libs`"
LIBFLAGS="-lllmozlib2 -lQtWebKit -lQtOpenGL -lnspr4 -lplc4 -lplds4 -lglui `pkg-config gtk+-2.0 --libs`"

rm -f ubrowser

${CXX} ${CXXOPT} ${INCFLAGS} app.cpp ubrowser.cpp ${LIBFLAGS} -o ubrowser

