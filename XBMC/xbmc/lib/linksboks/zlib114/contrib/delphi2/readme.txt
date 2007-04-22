These are files used to compile zlib under Borland C++ Builder 3.

zlib.bpg is the main project group that can be loaded in the BCB IDE and
loads all other *.bpr projects

zlib.bpr is a project used to create a static zlib.lib library with C calling
convention for functions.

zlib32.bpr creates a zlib32.dll dynamic link library with Windows standard
calling convention.

d_zlib.bpr creates a set of .obj files with register calling convention.
These files are used by zlib.pas to create a Delphi unit containing zlib.
The d_zlib.lib file generated isn't useful and can be deleted.

zlib.cpp, zlib32.cpp and d_zlib.cpp are used by the above projects.

