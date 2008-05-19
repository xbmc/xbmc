rm lib7z.a
make clean -f makefile.gcc
make -f makefile.gcc
rm 7zMain.o
ar r lib7z.a *.o
