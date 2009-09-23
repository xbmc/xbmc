
make

_OBJECTS="bit.o decoder.o fixed.o frame.o huffman.o layer3.o layer12.o stream.o synth.o timer.o"
_LIBS="-lstdc++"

#dlltool --export-all-symbols -z xbox/libmadnew.def $_OBJECTS
gcc -shared -o libmad.dll $_OBJECTS xbox/libmad.def $_LIBS -Wl,--out-implib,libmad_.lib
strip libmad.dll 

xbecopy libmad.dll xe:\\xbmc\\system\\players\\dvdplayer\\libmad.dll
