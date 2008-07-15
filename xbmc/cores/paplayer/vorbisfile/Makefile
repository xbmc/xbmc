vorbisfile-i486-linux.so:
	make -C libvorbis
	make -C ogg
	gcc -shared -o $@ ogg/src/*.o libvorbis/lib/*.o `cat ../../DllLoader/exports/wrapper.def` ../../DllLoader/exports/wrapper.o && cp $@ ../../../../system/players/paplayer

clean:
	make -C libvorbis clean
	make -C ogg clean
	rm -f vorbisfile-i486-linux.so

distclean:
	make -C libvorbis distclean
	make -C ogg distclean
	rm -f vorbisfile-i486-linux.so

