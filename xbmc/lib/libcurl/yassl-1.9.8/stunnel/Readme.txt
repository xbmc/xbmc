The stunnel MSVC 6.0 sp5 project workspace allows stunnel 4.05 to be built
with yaSSL providing SSL support.

Build *********
-----
Copy stunnel.dsp and .dsw into the stunnel root directory and then copy 
src/ssl.c and src/client.c into the stunnel/src directory.

ssl.c and client.c each have a YASSL #define for two features not currently
supported by yaSSL.

When yaSSL can fully support stunnel makefiles and workspaces will be submitted
to the project.

