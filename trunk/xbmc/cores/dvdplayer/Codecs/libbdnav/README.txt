
Generally, you do no check-in content that is generated. Like Binaries, *.o files etc, this includes Makefile.in, configure and so on. So when you check-out sources, first you run

autoreconf --force --install

and you get ./configure.


