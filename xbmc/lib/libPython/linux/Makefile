
python24-i486-linux.so : wrapper_python.o /usr/lib/python2.4/config/libpython2.4-pic.a wrapper_python.def
	if [ \! -d tmp ]; then mkdir tmp; fi
	(cd ./tmp; ar x /usr/lib/python2.4/config/libpython2.4-pic.a)
	gcc -shared -o $@ tmp/*.o `cat wrapper_python.def` wrapper_python.o
	rm -rf tmp
	cp -f $@ ../../../../system/python/

wrapper_python.o : wrapper_python.c
	gcc -fpic -o $@ -D_FILE_DEFINED -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -c $<

wrapper_python.def : wrapper_python.c
	(echo -n "-Wl"; grep __wrap wrapper_python.c | grep -v bash | sed "s/.*__wrap_//g" | sed "s/(.*//g" | awk '{printf(",-wrap,%s",$$0);}') > wrapper_python.def

clean:
	$(RM) *.so *.o *.def
