updating dll's

general note.

- When updating Dll's to a newer / older version, just make sure you update the header files
  in the xbmc source too.
- Our dll loader does not support ICL compiled dll's


oggvorbis.
--------------
you need 2 dll's for this, ogg.dll and vorbis.dll.
Both can be get from http://www.rarewares.org/ogg.html (you need the 'ogg vorbis dlls using libVorbis, for WinLame and CDex')


lame.
--------------
you need one dll for this which has to be compiled by mingw.
just use the two build scripts that reside in the xbox dir (compile.sh and configure.sh) and it should all work
