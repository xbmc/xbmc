@echo off
rem  ---------------------------------------------
rem  PURPOSE:
rem  - put this Batch-Command on your Desktop, 
rem    so you can drag and drop wave files on it
rem    and LAME will encode them to mp3 format.
rem  - put this Batch-Command in a place mentioned
rem    in your PATH environment, start the DOS-BOX
rem    and change to a directory where your wave 
rem    files are located. the following line will
rem    encode all your wave files to mp3
rem     "lame.bat *.wav"
rem  ---------------------------------------------
rem                         C2000  Robert Hegemann
rem  ---------------------------------------------
rem  Changes to support long filenames using 4DOS
rem  by Alexander Stumpf <dropdachalupa@gmx.net>
rem  ---------------------------------------------
rem  please set LAME and LAMEOPTS
rem  LAME - where the executeable is
rem  OPTS - options you like LAME to use

        set LAME=c:\progra~1\sound&~1\lame\lame.exe
        set OPTS=-h --lowpass-width 2 --lowpass 20.5 -b 112 --abr 180

rem  ---------------------------------------------

	set thecmd=%LAME% %OPTS%
        for %%f in (%&) do (%thecmd% %@sfn[%%f]^(ren %@sfn[%%f].mp3 "%@lfn[%%f].mp_">NUL))
        ren *.mp3.mp_ *.new.mp3 >& NUL
        ren *.wav.mp_ *.mp3 >& NUL
        goto endmark
:errormark
	echo.
	echo.
	echo ERROR processing %1
	echo. 
:endmark
rem
rem	finished
rem
