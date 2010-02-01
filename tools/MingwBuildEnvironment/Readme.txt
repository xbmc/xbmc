Mingw build environment to build ffmpeg and other libraries

Installation:
extract msys.7z to c:\
edit c:\msys\etc\fstab if you installed it in a different directory
edit c:\msys\msys.bat and adapt the path to vcvars32.bat (line 15)

Open a shell:
run c:\msys\msys.bat -rxvt

Source:
The environment was created like described here: http://ffmpeg.arrozcru.org/wiki/index.php?title=Main_Page