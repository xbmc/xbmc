@echo.
@echo This is a very simple batch file that makes debug and release
@echo versions of LLQtWebKit and then makes release versions of 
@echo testGL, uBrowser and QtTestApp. 
@echo.
@echo To make the Win32GL test, run the copy_files.bat batch file after
@echo running this one and load the MSVC solution file and build as normal.
@echo.
@echo This probably won't work unless you run it from a Qt command prompt
@echo since it needs a path to the Qt build directory.
@echo.
@pause

@rem Uncomment the next line if you DO NOT want to erase intermediate files first
@rem goto NO_ERASE

@rem Delete LLQtWebKit files
@rmdir .moc\ /s /q
@rmdir .obj\ /s /q
@rmdir .ui\ /s /q
@del Makefile
@del Makefile.Debug
@del Makefile.Release
@rmdir debug\ /s /q
@rmdir release\ /s /q

@rem Delete GL, GLUT and GLUI files
@rmdir tests\GL\ /s /q

@rem Delete QtTestApp files
@rmdir tests\qttestapp\Debug\ /s /q
@rmdir tests\qttestapp\Release\ /s /q
@del tests\qttestapp\Makefile
@del tests\qttestapp\Makefile.Debug
@del tests\qttestapp\Makefile.Release
@del tests\qttestapp\ui_window.h

@rem Delete testGL files
@rmdir tests\testgl\Debug\ /s /q
@rmdir tests\testgl\Release\ /s /q
@del tests\testgl\Makefile
@del tests\testgl\Makefile.Debug
@del tests\testgl\Makefile.Release

@rem Delete uBrowserfiles
@rmdir tests\ubrowser\Debug\ /s /q
@rmdir tests\ubrowser\Release\ /s /q
@del tests\ubrowser\Makefile
@del tests\ubrowser\Makefile.Debug
@del tests\ubrowser\Makefile.Release

:NO_ERASE

@rem location of GLUT and GLUI components we built previously
set GL_COMPONENT_DIR=C:\Work\qt\GL
xcopy %GL_COMPONENT_DIR%\*.* tests\GL\ /y

@rem clean and make a debug version of LLQtWebKit
nmake clean
qmake CONFIG+=debug
nmake clean
nmake

@rem clean and make a release version of LLQtWebKit
nmake clean
qmake CONFIG-=debug
nmake clean
nmake

@rem clean and make a release version of testGL test app
pushd .
cd tests\testgl
nmake clean
qmake CONFIG-=debug
nmake clean
nmake
popd

@rem clean and make a release version of QtTestApp test app
pushd .
cd tests\qttestapp
nmake clean
qmake CONFIG-=debug
nmake clean
nmake
popd

@rem clean and make a release version of uBrowser test app
pushd .
cd tests\ubrowser
nmake clean
qmake CONFIG-=debug
nmake clean
nmake
popd

@rem switch to root of tests directory - some need to be run from their own dir
cd tests

pause