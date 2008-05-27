
MADCodec-i486-linux.so: libmad-compile MADCodec/MadMp3Decoder.o
	gcc -shared -o $@  MADCodec/MadMp3Decoder.o libmad-0.15.1b/.libs/libmad.a `cat ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.def` ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.o -fPIC

libmad-compile:
	(cd libmad-0.15.1b; ./configure CFLAGS="-O3 -fPIC" CXXFLAGS="-O3 -fPIC")
	$(MAKE) -C libmad-0.15.1b
	
MADCodec/MadMp3Decoder.o: MADCodec/MadMp3Decoder.cpp
	g++ -o $@ -c $< -Ilibmad-0.15.1b -D_LINUX -fPIC

clean:
	$(MAKE) -C libmad-0.15.1b clean
	$(RM) MADCodec/MadMp3Decoder.o
	$(RM) MADCodec-i486-linux.so
