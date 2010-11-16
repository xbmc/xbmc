Author of sh4 specifics
-----------------------
Fredrik Ehnbom - original version in May, 2004
Fredrik Ehnbom - updated version for 2.0.0wip2 in Jan, 2005

Howto use
-----------------------
This assumes a working kallistios installation. It was developed with
kallistios 1.3.x subversion revision 183 and sh-elf-gcc 3.4.3. Though,
it does not really rely on anything kallistios specific, so it should
be possible to get it up and running on other sh4-based architectures.

AngelScript uses memory.h which does not seem to be provided with kos.
Don't worry though, the makefile creates one for you ;)

To build the library, just go to angelscript/source and type

make -f ../projects/dreamcast/Makefile

or wherever you put the makefile shipped with this textdocument.
The library will be in $KOS_BASE/addons/lib/dreamcast/ together with
all the other kos addon libraries.
