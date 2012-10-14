@echo off
cls
SET OutDir=s:\temp
SET Flags=-c -6 -O2 -Ve -X- -pr -a8 -b -d -k- -vi -tWM -r -RT- -n%OutDir%

del %OutDir%\*.obj
bcc32.exe %Flags% adler32.c
bcc32.exe %Flags% deflate.c
bcc32.exe %Flags% infback.c
bcc32.exe %Flags% inffast.c
bcc32.exe %Flags% inflate.c
bcc32.exe %Flags% inftrees.c
bcc32.exe %Flags% trees.c
bcc32.exe %Flags% crc32.c
bcc32.exe %Flags% compress.c