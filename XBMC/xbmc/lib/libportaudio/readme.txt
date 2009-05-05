libportaudio-osx.a is build from source code in source/portaudio.

./configure --disable-mac-universal && make

ditto lib/.libs/libportaudio.a -arch ppc -arch i386 xbmc/lib/libportaudio/libportaudio-osx.a