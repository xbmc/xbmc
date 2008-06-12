make



_OBJECTS="liba52/bitstream.o liba52/bit_allocate.o liba52/downmix.o liba52/imdct.o liba52/parse.o"

#create .def file
dlltool --export-all-symbols -z xbox/liba52.def liba52/.libs/liba52.a

#create .dll file
gcc -shared -o xbox/liba52.dll liba52/.libs/liba52.a $_OBJECTS xbox/liba52.def $_LIBS -Wl,--out-implib,xbox/liba52_.lib
strip xbox/liba52.dll 

