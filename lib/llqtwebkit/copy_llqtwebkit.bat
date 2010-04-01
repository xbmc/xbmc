@rem ===== source/destination dirs that may change =====
@set QT_SRC_DIR=C:\Work\qt\lindenqt
@set OPENSSL_SRC_DIR=c:\work\qt\openssl-0.9.8l
@set LLQTWEBKIT_SRC_DIR=C:\Work\llqtwebkit
@set LLQTWEBKIT_LICENSE_DIR=C:\Work\llqtwebkit\licenses
@set SL_DEST_DIR="C:\Documents and Settings\callum\Desktop"

@rem ===== library filename - change this when you make a new build but don't check in =====
@set LLQTWEBKIT_LIB_FILENAME="llqtwebkit-windows-qt4.6-2010XXXX.tar.bz2"

@rem ===== make sure the filename for the archive is updated =====
@if %LLQTWEBKIT_LIB_FILENAME%=="llqtwebkit-windows-qt4.6-2010XXXX.tar.bz2" goto VERSION_UNSET
@goto VERSION_SET
@:VERSION_UNSET
@echo ERROR: ****** The version number of the LLQtWebKit archive is not set
@goto END
@:VERSION_SET

@rem =============== instructions ===============
@echo This batch file copies the Qt, OpenSSL and LLQtWebKit files required 
@echo by LLQtWebKit into a Second Life client compatible source tree.
@echo.
@echo LLQtWebKit tarball will be: %LLQTWEBKIT_LIB_FILENAME%
@echo.
@echo ******* IMPORTANT ******* 
@echo.
@echo Check the following before proceeding:
@echo.
@echo   1/ You built release and debug versions of LLQtWebKit
@echo   2/ Directory paths in the settings above are correct
@echo   3/ LLQtWebKit library filename in the settings above is correct
@echo   4/ You have a recent version of 'tar' and 'md5sum' in your path
@echo.
@pause

@rem =============== Qt core libs and dlls for debug and release ===============
@if exist %QT_SRC_DIR%\lib\QtCore4.dll         (xcopy %QT_SRC_DIR%\lib\QtCore4.dll         %SL_DEST_DIR%\libraries\i686-win32\lib\release\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\lib\QtCore4.dll
@if exist %QT_SRC_DIR%\lib\QtCore4.lib         (xcopy %QT_SRC_DIR%\lib\QtCore4.lib         %SL_DEST_DIR%\libraries\i686-win32\lib\release\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\lib\QtCore4.lib
@if exist %QT_SRC_DIR%\lib\QtCored4.dll        (xcopy %QT_SRC_DIR%\lib\QtCored4.dll        %SL_DEST_DIR%\libraries\i686-win32\lib\debug\ /y)     else echo ****** MISSING: %QT_SRC_DIR%\lib\QtCored4.dll
@if exist %QT_SRC_DIR%\lib\QtCored4.lib        (xcopy %QT_SRC_DIR%\lib\QtCored4.lib        %SL_DEST_DIR%\libraries\i686-win32\lib\debug\ /y)     else echo ****** MISSING: %QT_SRC_DIR%\lib\QtCored4.lib
@if exist %QT_SRC_DIR%\lib\QtGui4.dll          (xcopy %QT_SRC_DIR%\lib\QtGui4.dll          %SL_DEST_DIR%\libraries\i686-win32\lib\release\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\lib\QtGui4.dll
@if exist %QT_SRC_DIR%\lib\QtGui4.lib          (xcopy %QT_SRC_DIR%\lib\QtGui4.lib          %SL_DEST_DIR%\libraries\i686-win32\lib\release\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\lib\QtGui4.lib
@if exist %QT_SRC_DIR%\lib\QtGuid4.dll         (xcopy %QT_SRC_DIR%\lib\QtGuid4.dll         %SL_DEST_DIR%\libraries\i686-win32\lib\debug\ /y)     else echo ****** MISSING: %QT_SRC_DIR%\lib\QtGuid4.dll
@if exist %QT_SRC_DIR%\lib\QtGuid4.lib         (xcopy %QT_SRC_DIR%\lib\QtGuid4.lib         %SL_DEST_DIR%\libraries\i686-win32\lib\debug\ /y)     else echo ****** MISSING: %QT_SRC_DIR%\lib\QtGuid4.lib
@if exist %QT_SRC_DIR%\lib\QtNetwork4.dll      (xcopy %QT_SRC_DIR%\lib\QtNetwork4.dll      %SL_DEST_DIR%\libraries\i686-win32\lib\release\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\lib\QtNetwork4.dll
@if exist %QT_SRC_DIR%\lib\QtNetwork4.lib      (xcopy %QT_SRC_DIR%\lib\QtNetwork4.lib      %SL_DEST_DIR%\libraries\i686-win32\lib\release\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\lib\QtNetwork4.lib
@if exist %QT_SRC_DIR%\lib\QtNetworkd4.dll     (xcopy %QT_SRC_DIR%\lib\QtNetworkd4.dll     %SL_DEST_DIR%\libraries\i686-win32\lib\debug\ /y)     else echo ****** MISSING: %QT_SRC_DIR%\lib\QtNetworkd4.dll
@if exist %QT_SRC_DIR%\lib\QtNetworkd4.lib     (xcopy %QT_SRC_DIR%\lib\QtNetworkd4.lib     %SL_DEST_DIR%\libraries\i686-win32\lib\debug\ /y)     else echo ****** MISSING: %QT_SRC_DIR%\lib\QtNetworkd4.lib
@if exist %QT_SRC_DIR%\lib\QtOpenGL4.dll       (xcopy %QT_SRC_DIR%\lib\QtOpenGL4.dll       %SL_DEST_DIR%\libraries\i686-win32\lib\release\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\lib\QtOpenGL4.dll
@if exist %QT_SRC_DIR%\lib\QtOpenGL4.lib       (xcopy %QT_SRC_DIR%\lib\QtOpenGL4.lib       %SL_DEST_DIR%\libraries\i686-win32\lib\release\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\lib\QtOpenGL4.lib
@if exist %QT_SRC_DIR%\lib\QtOpenGLd4.dll      (xcopy %QT_SRC_DIR%\lib\QtOpenGLd4.dll      %SL_DEST_DIR%\libraries\i686-win32\lib\debug\ /y)     else echo ****** MISSING: %QT_SRC_DIR%\lib\QtOpenGLd4.dll
@if exist %QT_SRC_DIR%\lib\QtOpenGLd4.lib      (xcopy %QT_SRC_DIR%\lib\QtOpenGLd4.lib      %SL_DEST_DIR%\libraries\i686-win32\lib\debug\ /y)     else echo ****** MISSING: %QT_SRC_DIR%\lib\QtOpenGLd4.lib
@if exist %QT_SRC_DIR%\lib\QtWebKit4.dll       (xcopy %QT_SRC_DIR%\lib\QtWebKit4.dll       %SL_DEST_DIR%\libraries\i686-win32\lib\release\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\lib\QtWebKit4.dll
@if exist %QT_SRC_DIR%\lib\QtWebKit4.lib       (xcopy %QT_SRC_DIR%\lib\QtWebKit4.lib       %SL_DEST_DIR%\libraries\i686-win32\lib\release\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\lib\QtWebKit4.lib
@if exist %QT_SRC_DIR%\lib\QtWebKitd4.dll      (xcopy %QT_SRC_DIR%\lib\QtWebKitd4.dll      %SL_DEST_DIR%\libraries\i686-win32\lib\debug\ /y)     else echo ****** MISSING: %QT_SRC_DIR%\lib\QtWebKitd4.dll
@if exist %QT_SRC_DIR%\lib\QtWebKitd4.lib      (xcopy %QT_SRC_DIR%\lib\QtWebKitd4.lib      %SL_DEST_DIR%\libraries\i686-win32\lib\debug\ /y)     else echo ****** MISSING: %QT_SRC_DIR%\lib\QtWebKitd4.lib
@if exist %QT_SRC_DIR%\lib\QtXmlPatterns4.dll  (xcopy %QT_SRC_DIR%\lib\QtXmlPatterns4.dll  %SL_DEST_DIR%\libraries\i686-win32\lib\release\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\lib\QtXmlPatterns4.dll
@if exist %QT_SRC_DIR%\lib\QtXmlPatterns4.lib  (xcopy %QT_SRC_DIR%\lib\QtXmlPatterns4.lib  %SL_DEST_DIR%\libraries\i686-win32\lib\release\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\lib\QtXmlPatterns4.lib
@if exist %QT_SRC_DIR%\lib\QtXmlPatternsd4.dll (xcopy %QT_SRC_DIR%\lib\QtXmlPatternsd4.dll %SL_DEST_DIR%\libraries\i686-win32\lib\debug\ /y)     else echo ****** MISSING: %QT_SRC_DIR%\lib\QtXmlPatternsd4.dll
@if exist %QT_SRC_DIR%\lib\QtXmlPatternsd4.lib (xcopy %QT_SRC_DIR%\lib\QtXmlPatternsd4.lib %SL_DEST_DIR%\libraries\i686-win32\lib\debug\ /y)     else echo ****** MISSING: %QT_SRC_DIR%\lib\QtXmlPatternsd4.lib
@if exist %QT_SRC_DIR%\lib\qtmain.lib          (xcopy %QT_SRC_DIR%\lib\qtmain.lib          %SL_DEST_DIR%\libraries\i686-win32\lib\release\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\lib\qtmain.lib
@if exist %QT_SRC_DIR%\lib\qtmaind.lib         (xcopy %QT_SRC_DIR%\lib\qtmaind.lib         %SL_DEST_DIR%\libraries\i686-win32\lib\debug\ /y)     else echo ****** MISSING: %QT_SRC_DIR%\lib\qtmaind.lib

@rem =============== Qt imageformat plugin dlls for debug and release ===============
@if exist %QT_SRC_DIR%\plugins\imageformats\qgifd4.dll  (xcopy %QT_SRC_DIR%\plugins\imageformats\qgifd4.dll  %SL_DEST_DIR%\libraries\i686-win32\lib\debug\imageformats\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\plugins\imageformats\qgifd4.dll
@if exist %QT_SRC_DIR%\plugins\imageformats\qicod4.dll  (xcopy %QT_SRC_DIR%\plugins\imageformats\qicod4.dll  %SL_DEST_DIR%\libraries\i686-win32\lib\debug\imageformats\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\plugins\imageformats\qicod4.dll
@if exist %QT_SRC_DIR%\plugins\imageformats\qjpegd4.dll (xcopy %QT_SRC_DIR%\plugins\imageformats\qjpegd4.dll %SL_DEST_DIR%\libraries\i686-win32\lib\debug\imageformats\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\plugins\imageformats\qjpegd4.dll
@if exist %QT_SRC_DIR%\plugins\imageformats\qmngd4.dll  (xcopy %QT_SRC_DIR%\plugins\imageformats\qmngd4.dll  %SL_DEST_DIR%\libraries\i686-win32\lib\debug\imageformats\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\plugins\imageformats\qmngd4.dll
@if exist %QT_SRC_DIR%\plugins\imageformats\qsvgd4.dll  (xcopy %QT_SRC_DIR%\plugins\imageformats\qsvgd4.dll  %SL_DEST_DIR%\libraries\i686-win32\lib\debug\imageformats\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\plugins\imageformats\qsvgd4.dll
@if exist %QT_SRC_DIR%\plugins\imageformats\qtiffd4.dll (xcopy %QT_SRC_DIR%\plugins\imageformats\qtiffd4.dll %SL_DEST_DIR%\libraries\i686-win32\lib\debug\imageformats\ /y)   else echo ****** MISSING: %QT_SRC_DIR%\plugins\imageformats\qtiffd4.dll
@if exist %QT_SRC_DIR%\plugins\imageformats\qgif4.dll   (xcopy %QT_SRC_DIR%\plugins\imageformats\qgif4.dll   %SL_DEST_DIR%\libraries\i686-win32\lib\release\imageformats\ /y) else echo ****** MISSING: %QT_SRC_DIR%\plugins\imageformats\qgif4.dll
@if exist %QT_SRC_DIR%\plugins\imageformats\qico4.dll   (xcopy %QT_SRC_DIR%\plugins\imageformats\qico4.dll   %SL_DEST_DIR%\libraries\i686-win32\lib\release\imageformats\ /y) else echo ****** MISSING: %QT_SRC_DIR%\plugins\imageformats\qico4.dll
@if exist %QT_SRC_DIR%\plugins\imageformats\qjpeg4.dll  (xcopy %QT_SRC_DIR%\plugins\imageformats\qjpeg4.dll  %SL_DEST_DIR%\libraries\i686-win32\lib\release\imageformats\ /y) else echo ****** MISSING: %QT_SRC_DIR%\plugins\imageformats\qjpeg4.dll
@if exist %QT_SRC_DIR%\plugins\imageformats\qmng4.dll   (xcopy %QT_SRC_DIR%\plugins\imageformats\qmng4.dll   %SL_DEST_DIR%\libraries\i686-win32\lib\release\imageformats\ /y) else echo ****** MISSING: %QT_SRC_DIR%\plugins\imageformats\qmng4.dll
@if exist %QT_SRC_DIR%\plugins\imageformats\qsvg4.dll   (xcopy %QT_SRC_DIR%\plugins\imageformats\qsvg4.dll   %SL_DEST_DIR%\libraries\i686-win32\lib\release\imageformats\ /y) else echo ****** MISSING: %QT_SRC_DIR%\plugins\imageformats\qsvg4.dll
@if exist %QT_SRC_DIR%\plugins\imageformats\qtiff4.dll  (xcopy %QT_SRC_DIR%\plugins\imageformats\qtiff4.dll  %SL_DEST_DIR%\libraries\i686-win32\lib\release\imageformats\ /y) else echo ****** MISSING: %QT_SRC_DIR%\plugins\imageformats\qtiff4.dll

@rem =============== Qt codec plugin dlls for debug and release ===============
@if exist %QT_SRC_DIR%\plugins\codecs\qcncodecs4.dll  (xcopy %QT_SRC_DIR%\plugins\codecs\qcncodecs4.dll %SL_DEST_DIR%\libraries\i686-win32\lib\release\codecs\ /y)  else echo ****** MISSING: %QT_SRC_DIR%\plugins\codecs\qcncodecs4.dll
@if exist %QT_SRC_DIR%\plugins\codecs\qcncodecsd4.dll (xcopy %QT_SRC_DIR%\plugins\codecs\qcncodecsd4.dll %SL_DEST_DIR%\libraries\i686-win32\lib\debug\codecs\ /y) else echo ****** MISSING: %QT_SRC_DIR%\plugins\codecs\qcncodecsd4.dll
@if exist %QT_SRC_DIR%\plugins\codecs\qjpcodecs4.dll  (xcopy %QT_SRC_DIR%\plugins\codecs\qjpcodecs4.dll %SL_DEST_DIR%\libraries\i686-win32\lib\release\codecs\ /y)  else echo ****** MISSING: %QT_SRC_DIR%\plugins\codecs\qjpcodecs4.dll
@if exist %QT_SRC_DIR%\plugins\codecs\qjpcodecsd4.dll (xcopy %QT_SRC_DIR%\plugins\codecs\qjpcodecsd4.dll %SL_DEST_DIR%\libraries\i686-win32\lib\debug\codecs\ /y) else echo ****** MISSING: %QT_SRC_DIR%\plugins\codecs\qjpcodecsd4.dll
@if exist %QT_SRC_DIR%\plugins\codecs\qkrcodecs4.dll  (xcopy %QT_SRC_DIR%\plugins\codecs\qkrcodecs4.dll %SL_DEST_DIR%\libraries\i686-win32\lib\release\codecs\ /y)  else echo ****** MISSING: %QT_SRC_DIR%\plugins\codecs\qkrcodecs4.dll
@if exist %QT_SRC_DIR%\plugins\codecs\qkrcodecsd4.dll (xcopy %QT_SRC_DIR%\plugins\codecs\qkrcodecsd4.dll %SL_DEST_DIR%\libraries\i686-win32\lib\debug\codecs\ /y) else echo ****** MISSING: %QT_SRC_DIR%\plugins\codecs\qkrcodecsd4.dll
@if exist %QT_SRC_DIR%\plugins\codecs\qtwcodecs4.dll  (xcopy %QT_SRC_DIR%\plugins\codecs\qtwcodecs4.dll %SL_DEST_DIR%\libraries\i686-win32\lib\release\codecs\ /y)  else echo ****** MISSING: %QT_SRC_DIR%\plugins\codecs\qtwcodecs4.dll
@if exist %QT_SRC_DIR%\plugins\codecs\qtwcodecsd4.dll (xcopy %QT_SRC_DIR%\plugins\codecs\qtwcodecsd4.dll %SL_DEST_DIR%\libraries\i686-win32\lib\debug\codecs\ /y) else echo ****** MISSING: %QT_SRC_DIR%\plugins\codecs\qtwcodecsd4.dll

@rem =============== OpenSSL dlls ===============
@if exist %OPENSSL_SRC_DIR%\out32dll\ssleay32.dll (xcopy %OPENSSL_SRC_DIR%\out32dll\ssleay32.dll %SL_DEST_DIR%\libraries\i686-win32\lib\debug\ /y)   else echo ****** MISSING: %OPENSSL_SRC_DIR%\out32dll\ssleay32.dll
@if exist %OPENSSL_SRC_DIR%\out32dll\libeay32.dll (xcopy %OPENSSL_SRC_DIR%\out32dll\libeay32.dll %SL_DEST_DIR%\libraries\i686-win32\lib\debug\ /y)   else echo ****** MISSING: %OPENSSL_SRC_DIR%\out32dll\libeay32.dll
@if exist %OPENSSL_SRC_DIR%\out32dll\ssleay32.dll (xcopy %OPENSSL_SRC_DIR%\out32dll\ssleay32.dll %SL_DEST_DIR%\libraries\i686-win32\lib\release\ /y) else echo ****** MISSING: %OPENSSL_SRC_DIR%\out32dll\ssleay32.dll
@if exist %OPENSSL_SRC_DIR%\out32dll\libeay32.dll (xcopy %OPENSSL_SRC_DIR%\out32dll\libeay32.dll %SL_DEST_DIR%\libraries\i686-win32\lib\release\ /y) else echo ****** MISSING: %OPENSSL_SRC_DIR%\out32dll\libeay32.dll

@rem =============== LLQtWebKit header and libs for debug and release ===============
@if exist %LLQTWEBKIT_SRC_DIR%\llqtwebkit.h           (xcopy %LLQTWEBKIT_SRC_DIR%\llqtwebkit.h           %SL_DEST_DIR%\libraries\include\ /y)                else echo ****** MISSING: %LLQTWEBKIT_SRC_DIR%\llqtwebkit.h
@if exist %LLQTWEBKIT_SRC_DIR%\debug\llqtwebkitd.lib  (xcopy %LLQTWEBKIT_SRC_DIR%\debug\llqtwebkitd.lib  %SL_DEST_DIR%\libraries\i686-win32\lib\debug\ /y)   else echo ****** MISSING: %LLQTWEBKIT_SRC_DIR%\debug\llqtwebkitd.lib
@if exist %LLQTWEBKIT_SRC_DIR%\release\llqtwebkit.lib (xcopy %LLQTWEBKIT_SRC_DIR%\release\llqtwebkit.lib %SL_DEST_DIR%\libraries\i686-win32\lib\release\ /y) else echo ****** MISSING: %LLQTWEBKIT_SRC_DIR%\release\llqtwebkit.lib

@rem =============== LLQtWebKit and Qt license files ===============
@if exist %LLQTWEBKIT_LICENSE_DIR%\qt-4.6-LGPL_EXCEPTION.txt (xcopy %LLQTWEBKIT_LICENSE_DIR%\qt-4.6-LGPL_EXCEPTION.txt %SL_DEST_DIR%\LICENSES\ /y) else echo ****** MISSING: %LLQTWEBKIT_LICENSE_DIR%\qt-4.6-LGPL_EXCEPTION.txt
@if exist %LLQTWEBKIT_LICENSE_DIR%\qt-4.6-LICENSE.LGPL       (xcopy %LLQTWEBKIT_LICENSE_DIR%\qt-4.6-LICENSE.LGPL       %SL_DEST_DIR%\LICENSES\ /y) else echo ****** MISSING: %LLQTWEBKIT_LICENSE_DIR%\qt-4.6-LICENSE.LGPL
@if exist %LLQTWEBKIT_LICENSE_DIR%\qt-4.6-linden-changes.txt (xcopy %LLQTWEBKIT_LICENSE_DIR%\qt-4.6-linden-changes.txt %SL_DEST_DIR%\LICENSES\ /y) else echo ****** MISSING: %LLQTWEBKIT_LICENSE_DIR%\qt-4.6-linden-changes.txt

@echo.
@echo Generating tarball: %LLQTWEBKIT_LIB_FILENAME%
@echo.
@cd %SL_DEST_DIR%
@tar czspf %LLQTWEBKIT_LIB_FILENAME% libraries\ LICENSES\

@echo Generating MD5 sum for the tarball
@echo.
@md5sum -b %LLQTWEBKIT_LIB_FILENAME%
@md5sum -b %LLQTWEBKIT_LIB_FILENAME% > %LLQTWEBKIT_LIB_FILENAME%.md5sum
@echo.

@echo -- Complete --
:END
@pause
