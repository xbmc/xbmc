: Add version info resource with some strings and a resource file to verpatch.exe
: (also kind of self test)

: Run this in Release or Debug dir
:
set _ver="1.0.1.9 [%date%]"
set _s1=/s desc "Version patcher tool" /s copyright "(C) 1998-2011, pavel_a"
set _s1=%_s1% /s pb "pa"
set _s1=%_s1% /pv "1.0.0.1 (free)" 

set _resfile=/rf #64 ..\usage.txt

: Run a copy of verpatch on itself:

copy verpatch.exe v.exe || exit /b 1

v.exe verpatch.exe /va %_ver% %_s1% %_s2% %_resfile% 

@echo Errorlevel=%errorlevel%
