
cd libdts
make
cd ..

_OBJECTS="libdts/bitstream.o libdts/downmix.o libdts/parse.o"
_LIBS="-lstdc++"

#dlltool --export-all-symbols -z xbox/libdtsnew.def $_OBJECTS
gcc -shared -o libdts.dll $_OBJECTS xbox/libdts.def $_LIBS -Wl,--out-implib,libdts_.lib
strip libdts.dll 

xbecopy libdts.dll xe:\\xbmc\\system\\players\\dvdplayer\\libdts.dll
