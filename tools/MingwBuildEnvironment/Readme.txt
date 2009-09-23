Mingw build environment to build ffmpeg and other libraries

Installation:
extract msys.7z to c:\

Open a shell:
run c:\msys\msys.bat

Source:
The environment was compiled like described here: http://ffmpeg.arrozcru.org/wiki/index.php?title=Main_Page

Changes:
I had to adapt the file c:\msys\mingw\include\vfw.h in order to build without errors. This change might need to be reverted for later builds.
To do so open the file in an editor and remove the comments from line 673 to 708 (CAPTUREPARMS and VIDEOHDR are already defined in libavcodec).