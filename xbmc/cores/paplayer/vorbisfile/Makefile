vorbisfile-i486-linux.so:
	make -C libvorbis
	make -C ogg
	gcc -shared -o $@ ogg/src/.libs/*.o libvorbis/lib/.libs/*.o `cat ../../DllLoader/exports/wrapper.def` ../../DllLoader/exports/wrapper.o && cp $@ ../../../../system/players/paplayer

clean:
	if test -f libvorbis/Makefile; then (make -C libvorbis clean); fi;
	if test -f ogg/Makefile;       then (make -C ogg clean); fi;
	rm -f vorbisfile-i486-linux.so

distclean:
	if test -f libvorbis/Makefile; then (make -C libvorbis distclean); fi;
	if test -f ogg/Makefile;       then (make -C ogg distclean); fi;
	rm -f vorbisfile-i486-linux.so

