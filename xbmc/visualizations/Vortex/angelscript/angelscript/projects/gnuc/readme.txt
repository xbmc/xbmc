To compile the library, just type in the command line:

make

In order to install the library you might have to adopt the makefile to
match your system configuration. By default the library will be installed
to "/usr/local/..." (lib/ and include/). To change that, edit the makefile
variable $LOCAL.

In order to install the library, switch to root:

su
make install
exit

You can uninstall the library, too:

su
make uninstall
exit
