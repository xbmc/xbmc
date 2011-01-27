
ECHO mkdir dll ...
rmdir /s install\dll
mkdir install\dll

ECHO libvdvcss dll ...
xcopy /Y %1\bin\libdvdcss.dll install\dll

