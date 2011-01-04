@echo off
Rem Configure libiconv for DJGPP.

Rem WARNING WARNING WARNING: This file needs to have DOS CRLF end-of-line
Rem format, or else stock DOS/Windows shells will refuse to run it.

echo Configuring GNU libiconv for DJGPP v2.x...
Rem The SmallEnv tests protect against fixed and too small size
Rem of the environment in stock DOS shell.

Rem Find out if NLS is wanted or not,
Rem if dependency-tracking is wanted or not,
Rem if caching is wanted or not
Rem and where the sources are.
Rem We always default to NLS support,
Rem no dependency tracking
Rem and to in place configuration.
set ARGS=
set NLS=disabled
if not "%NLS%" == "disabled" goto SmallEnv
set CACHING=enabled
if not "%CACHING%" == "enabled" goto SmallEnv
set DEPENDENCY_TRACKING=disabled
if not "%DEPENDENCY_TRACKING%" == "disabled" goto SmallEnv
set LIBICONV_PREFIX=disabled
if not "%LIBICONV_PREFIX%" == "disabled" goto SmallEnv
set LIBINTL_PREFIX=disabled
if not "%LIBINTL_PREFIX%" == "disabled" goto SmallEnv
set HTML=enabled
if not "%HTML%" == "enabled" goto SmallEnv
set XSRC=.
if not "%XSRC%" == "." goto SmallEnv

Rem Loop over all arguments.
Rem Special arguments are: NLS, XSRC, CACHE, STATIC_LIBS, LIBICONV_PREFIX, LIBINTL_PREFIX and DEPS.
Rem All other arguments are stored into ARGS.
:ArgLoop
if "%1" == "nls" goto NextArgument
if "%1" == "NLS" goto NextArgument
if "%1" == "no-nls" goto NoNLS
if "%1" == "no-NLS" goto NoNLS
if "%1" == "NO-NLS" goto NoNLS
goto CachingOption
:NoNLS
if "%1" == "no-nls" set NLS=disabled
if "%1" == "no-NLS" set NLS=disabled
if "%1" == "NO-NLS" set NLS=disabled
if not "%NLS%" == "disabled" goto SmallEnv
goto NextArgument
:CachingOption
if "%1" == "cache" goto NextArgument
if "%1" == "CACHE" goto NextArgument
if "%1" == "no-cache" goto NoCaching
if "%1" == "no-CACHE" goto NoCaching
if "%1" == "NO-CACHE" goto NoCaching
goto DependencyOption
:NoCaching
if "%1" == "no-cache" set CACHING=disabled
if "%1" == "no-CACHE" set CACHING=disabled
if "%1" == "NO-CACHE" set CACHING=disabled
if not "%CACHING%" == "disabled" goto SmallEnv
goto NextArgument
:DependencyOption
if "%1" == "no-dep" goto NextArgument
if "%1" == "no-DEP" goto NextArgument
if "%1" == "NO-DEP" goto NextArgument
if "%1" == "dep" goto DependecyTraking
if "%1" == "DEP" goto DependecyTraking
goto LibiconvPrefixOption
:DependecyTraking
if "%1" == "dep" set DEPENDENCY_TRACKING=enabled
if "%1" == "DEP" set DEPENDENCY_TRACKING=enabled
if not "%DEPENDENCY_TRACKING%" == "enabled" goto SmallEnv
goto NextArgument
:LibiconvPrefixOption
if "%1" == "no-libiconvprefix" goto NextArgument
if "%1" == "no-LIBICONVPREFIX" goto NextArgument
if "%1" == "NO-LIBICONVPREFIX" goto NextArgument
if "%1" == "libiconvprefix" goto WithLibiconvPrefix
if "%1" == "LIBICONVPREFIX" goto WithLibiconvPrefix
goto LibintlPrefixOption
:WithLibiconvPrefix
if "%1" == "libiconvprefix" set LIBICONV_PREFIX=enabled
if "%1" == "LIBICONVPREFIX" set LIBICONV_PREFIX=enabled
if not "%LIBICONV_PREFIX%" == "enabled" goto SmallEnv
goto NextArgument
:LibintlPrefixOption
if "%1" == "no-libiconvprefix" goto NextArgument
if "%1" == "no-LIBICONVPREFIX" goto NextArgument
if "%1" == "NO-LIBICONVPREFIX" goto NextArgument
if "%1" == "libintlprefix" goto _WithLibintlPrefix
if "%1" == "LIBINTLPREFIX" goto _WithLibintlPrefix
goto HTMLOption
:_WithLibintlPrefix
if "%1" == "libintlprefix" set LIBINTL_PREFIX=enabled
if "%1" == "LIBINTLPREFIX" set LIBINTL_PREFIX=enabled
if not "%LIBINTL_PREFIX%" == "enabled" goto SmallEnv
:HTMLOption
if "%1" == "withhtml" goto NextArgument
if "%1" == "withHTML" goto NextArgument
if "%1" == "WITHHTML" goto NextArgument
if "%1" == "withouthtml" goto _WithoutHTML
if "%1" == "withoutHTML" goto _WithoutHTML
if "%1" == "WITHOUTHTML" goto _WithoutHTML
goto SrcDirOption
:_WithoutHTML
if "%1" == "withouthtml" set HTML=disabled
if "%1" == "withoutHTML" set HTML=disabled
if "%1" == "WITHOUTHTML" set HTML=disabled
if not "%HTML%" == "disabled" goto SmallEnv
goto NextArgument
:SrcDirOption
echo %1 | grep -q "/"
if errorlevel 1 goto CollectArgument
set XSRC=%1
if not "%XSRC%" == "%1" goto SmallEnv
goto NextArgument
:CollectArgument
set _ARGS=%ARGS% %1
if not "%_ARGS%" == "%ARGS% %1" if not "%_ARGS%" == "%ARGS%%1" goto SmallEnv
echo %_ARGS% | grep -q "[^ ]"
if not errorlevel 0 set ARGS=%_ARGS%
set _ARGS=
:NextArgument
shift
if not "%1" == "" goto ArgLoop

Rem Create an arguments file for the configure script.
echo --srcdir=%XSRC% > arguments
if "%CACHING%" == "enabled"              echo --cache-file=%XSRC%/djgpp/config.cache >> arguments
if "%DEPENDENCY_TRACKING%" == "enabled"  echo --enable-dependency-tracking >> arguments
if "%DEPENDENCY_TRACKING%" == "disabled" echo --disable-dependency-tracking >> arguments
if "%LIBICONV_PREFIX%" == "enabled"      echo --with-libiconv-prefix >> arguments
if "%LIBICONV_PREFIX%" == "disabled"     echo --without-libiconv-prefix >> arguments
if "%LIBINTL_PREFIX%" == "enabled"       echo --with-libintl-prefix >> arguments
if "%LIBINTL_PREFIX%" == "disabled"      echo --without-libintl-prefix >> arguments
if "%HTML%" == "enabled"                 echo --enable-html >> arguments
if "%HTML%" == "disabled"                echo --disable-html >> arguments
if not "%ARGS%" == ""                    echo %ARGS% >> arguments
set ARGS=
set CACHING=
set DEPENDENCY_TRACKING=
set LIBICONV_PREFIX=
set LIBINTL_PREFIX=
set HTML=

Rem Find out where the sources are
if "%XSRC%" == "." goto InPlace

:NotInPlace
redir -e /dev/null update %XSRC%/configure.org ./configure
test -f ./configure
if errorlevel 1 update %XSRC%/configure ./configure
test -d ./libcharset
if errorlevel 1 md libcharset
redir -e /dev/null update %XSRC%/libcharset/configure.org ./libcharset/configure
test -f ./libcharset/configure
if errorlevel 1 update %XSRC%/libcharset/configure ./libcharset/configure

:InPlace
Rem Update configuration files
echo Updating configuration scripts...
test -f ./configure.org
if errorlevel 1 update ./configure ./configure.org
sed -f %XSRC%/djgpp/config.sed ./configure.org > configure
if errorlevel 1 goto SedError
test -f ./libcharset/configure.org
if errorlevel 1 update ./libcharset/configure ./libcharset/configure.org
sed -f %XSRC%/djgpp/config.sed ./libcharset/configure.org > configure.tmp
if errorlevel 1 goto SedError
Rem The following is needed because the toplevel configure script calls the
Rem %XSRC%/libcharset/configure script instead of ./libcharset/configure.
test -f %XSRC%/libcharset/configure.org
if errorlevel 1 update %XSRC%/libcharset/configure %XSRC%/libcharset/configure.org
update configure.tmp %XSRC%/libcharset/configure
rm ./configure.tmp

Rem Make sure they have a config.site file
set CONFIG_SITE=%XSRC%/djgpp/config.site
if not "%CONFIG_SITE%" == "%XSRC%/djgpp/config.site" goto SmallEnv

Rem Make sure crucial file names are not munged by unpacking
test -f %XSRC%/config.h.in
if not errorlevel 1 redir -e /dev/null mv -f %XSRC%/config.h.in %XSRC%/config.h-in
test -f %XSRC%/config.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/config.h %XSRC%/config.h-in
test -f %XSRC%/config.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/configh.in %XSRC%/config.h-in
test -f %XSRC%/config.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/config_h.in %XSRC%/config.h-in
test -f %XSRC%/lib/config.h.in
if not errorlevel 1 redir -e /dev/null mv -f %XSRC%/lib/config.h.in %XSRC%/lib/config.h-in
test -f %XSRC%/lib/config.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/lib/config.h %XSRC%/lib/config.h-in
test -f %XSRC%/lib/config.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/lib/configh.in %XSRC%/lib/config.h-in
test -f %XSRC%/lib/config.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/lib/config_h.in %XSRC%/lib/config.h-in
test -f %XSRC%/include/iconv.h.in
if not errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/iconv.h.in %XSRC%/include/iconv.h-in
test -f %XSRC%/include/iconv.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/iconv.h %XSRC%/include/iconv.h-in
test -f %XSRC%/include/iconv.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/iconvh.in %XSRC%/include/iconv.h-in
test -f %XSRC%/include/iconv.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/iconv_h.in %XSRC%/include/iconv.h-in
test -f %XSRC%/include/iconv.h.build.in
if not errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/iconv.h.build.in %XSRC%/include/iconv.h-build-in
test -f %XSRC%/include/iconv.h-build-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/iconv.h %XSRC%/include/iconv.h-build-in
test -f %XSRC%/include/iconv.h-build-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/iconvh.build %XSRC%/include/iconv.h-build-in
test -f %XSRC%/include/iconv.h-build-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/iconv_h.build %XSRC%/include/iconv.h-build-in
test -f %XSRC%/libcharset/config.h.in
if not errorlevel 1 redir -e /dev/null mv -f %XSRC%/libcharset/config.h.in %XSRC%/libcharset/config.h-in
test -f %XSRC%/libcharset/config.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/libcharset/config.h %XSRC%/libcharset/config.h-in
test -f %XSRC%/libcharset/config.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/libcharset/configh.in %XSRC%/libcharset/config.h-in
test -f %XSRC%/libcharset/config.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/libcharset/config_h.in %XSRC%/libcharset/config.h-in
test -f %XSRC%/libcharset/include/libcharset.h.in
if not errorlevel 1 redir -e /dev/null mv -f %XSRC%/libcharset/include/libcharset.h.in %XSRC%/libcharset/include/libcharset.h-in
test -f %XSRC%/libcharset/include/libcharset.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/libcharset/include/libcharset.h %XSRC%/libcharset/include/libcharset.h-in
test -f %XSRC%/libcharset/include/libcharset.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/libcharset/include/libcharseth.in %XSRC%/libcharset/include/libcharset.h-in
test -f %XSRC%/libcharset/include/libcharset.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/libcharset/include/libcharset_h.in %XSRC%/libcharset/include/libcharset.h-in
test -f %XSRC%/libcharset/include/localcharset.h.in
if not errorlevel 1 redir -e /dev/null mv -f %XSRC%/libcharset/include/localcharset.h.in %XSRC%/libcharset/include/localcharset.h-in
test -f %XSRC%/libcharset/include/localcharset.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/libcharset/include/localcharset.h %XSRC%/libcharset/include/localcharset.h-in
test -f %XSRC%/libcharset/include/localcharset.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/libcharset/include/localcharseth.in %XSRC%/libcharset/include/localcharset.h-in
test -f %XSRC%/libcharset/include/localcharset.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/libcharset/include/localcharset_h.in %XSRC%/libcharset/include/localcharset.h-in
test -f %XSRC%/libcharset/include/localcharset.h.build.in
if not errorlevel 1 redir -e /dev/null mv -f %XSRC%/libcharset/include/localcharset.h.build.in %XSRC%/libcharset/include/localcharset.h-build-in
test -f %XSRC%/libcharset/include/localcharset.h-build-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/libcharset/include/localcharset.h %XSRC%/libcharset/include/localcharset.h-build-in
test -f %XSRC%/libcharset/include/localcharset.h-build-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/libcharset/include/localcharseth.build %XSRC%/libcharset/include/localcharset.h-build-in
test -f %XSRC%/libcharset/include/localcharset.h-build-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/libcharset/include/localcharset_h.build %XSRC%/libcharset/include/localcharset.h-build-in

Rem DJGPP needs ICONV_CONST set to const.
sed "s/^#undef ICONV_CONST/#define ICONV_CONST const/" %XSRC%/config.h-in > config.tmp
mv -f config.tmp %XSRC%/config.h-in

Rem All fixes needed to get the package configured, compiled and tested.
Rem 1:  Change the stateless-check script so it knowns about the
Rem     new filenames.
Rem 2:  Ditto for Makefile.in
Rem 3:  Ditto for source files.

:test -f %XSRC%/stamp-djgppfixes
:if not errorlevel 1 goto TestsuitFixed
Rem Fix the Makefile.ins.
test -f %XSRC%/lib/Makefile.org
if errorlevel 1 update %XSRC%/lib/Makefile.in %XSRC%/lib/Makefile.org
sed -f %XSRC%/djgpp/makefile.sed %XSRC%/lib/Makefile.org > Makefile.tmp
if errorlevel 1 goto SedError
update Makefile.tmp %XSRC%/lib/Makefile.in
rm Makefile.tmp
test -f %XSRC%/tests/Makefile.org
if errorlevel 1 update %XSRC%/tests/Makefile.in %XSRC%/tests/Makefile.org
sed -f %XSRC%/djgpp/makefile.sed %XSRC%/tests/Makefile.org > Makefile.tmp
if errorlevel 1 goto SedError
update Makefile.tmp %XSRC%/tests/Makefile.in
rm Makefile.tmp

Rem Fix the source files.
test -f %XSRC%/lib/aliases/aliases2.org
if errorlevel 1 update %XSRC%/lib/aliases/aliases2.h %XSRC%/lib/aliases/aliases2.org
sed -f %XSRC%/djgpp/sources.sed %XSRC%/lib/aliases/aliases2.org > aliases2.tmp
if errorlevel 1 goto SedError
update aliases2.tmp %XSRC%/lib/aliases/aliases2.h
rm aliases2.tmp
test -f %XSRC%/lib/iconv.org
if errorlevel 1 update %XSRC%/lib/iconv.c %XSRC%/lib/iconv.org
sed -f %XSRC%/djgpp/sources.sed %XSRC%/lib/iconv.org > iconv.tmp
if errorlevel 1 goto SedError
update iconv.tmp %XSRC%/lib/iconv.c
rm iconv.tmp
test -f %XSRC%/lib/converters.org
if errorlevel 1 update %XSRC%/lib/converters.h %XSRC%/lib/converters.org
sed -f %XSRC%/djgpp/sources.sed %XSRC%/lib/converters.org > converters.tmp
if errorlevel 1 goto SedError
update converters.tmp %XSRC%/lib/converters.h
rm converters.tmp
test -f %XSRC%/lib/cns/11643.org
if errorlevel 1 update %XSRC%/lib/cns/11643.h %XSRC%/lib/cns/11643.org
sed -f %XSRC%/djgpp/sources.sed %XSRC%/lib/cns/11643.org > 11643.tmp
if errorlevel 1 goto SedError
update 11643.tmp %XSRC%/lib/cns/11643.h
rm 11643.tmp
test -f %XSRC%/lib/cns/11643_4.org
if errorlevel 1 update %XSRC%/lib/cns/11643_4.h %XSRC%/lib/cns/11643_4.org
sed -f %XSRC%/djgpp/sources.sed %XSRC%/lib/cns/11643_4.org > 11643_4.tmp
if errorlevel 1 goto SedError
update 11643_4.tmp %XSRC%/lib/cns/11643_4.h
rm 11643_4.tmp
test -f %XSRC%/lib/iso/ir165.org
if errorlevel 1 update %XSRC%/lib/iso/ir165.h %XSRC%/lib/iso/ir165.org
sed -f %XSRC%/djgpp/sources.sed %XSRC%/lib/iso/ir165.org > ir165.tmp
if errorlevel 1 goto SedError
update ir165.tmp %XSRC%/lib/iso/ir165.h
rm ir165.tmp
test -f %XSRC%/lib/big5hkscs/1999.org
if errorlevel 1 update %XSRC%/lib/big5hkscs/1999.h %XSRC%/lib/big5hkscs/1999.org
sed -f %XSRC%/djgpp/sources.sed %XSRC%/lib/big5hkscs/1999.org > 1999.tmp
if errorlevel 1 goto SedError
update 1999.tmp %XSRC%/lib/big5hkscs/1999.h
rm 1999.tmp
test -f %XSRC%/lib/big5hkscs/2001.org
if errorlevel 1 update %XSRC%/lib/big5hkscs/2001.h %XSRC%/lib/big5hkscs/2001.org
sed -f %XSRC%/djgpp/sources.sed %XSRC%/lib/big5hkscs/2001.org > 2001.tmp
if errorlevel 1 goto SedError
update 2001.tmp %XSRC%/lib/big5hkscs/2001.h
rm 2001.tmp
test -f %XSRC%/lib/big5hkscs/2004.org
if errorlevel 1 update %XSRC%/lib/big5hkscs/2004.h %XSRC%/lib/big5hkscs/2004.org
sed -f %XSRC%/djgpp/sources.sed %XSRC%/lib/big5hkscs/2004.org > 2004.tmp
if errorlevel 1 goto SedError
update 2004.tmp %XSRC%/lib/big5hkscs/2004.h
rm 2004.tmp

Rem Fix the test scripts.
if "%XSRC%" == "." goto NoDirChange
cd | sed "s|:.*$|:|" > cd_BuildDir.bat
cd | sed "s|^.:|cd |" >> cd_BuildDir.bat
mv -f cd_BuildDir.bat %XSRC%/cd_BuildDir.bat
echo %XSRC% | sed -e "s|^/dev/||" -e "s|/|:|" -e "s|:.*$|:|g" > cd_SrcDir.bat
echo %XSRC% | sed -e "s|^/dev/||" -e "s|/|:/|" -e "s|^.*:|cd |" -e "s|^\.\.|cd &|" -e "s|/|\\|g" >> cd_SrcDir.bat
call cd_SrcDir.bat
call djgpp\edtest.bat
call cd_BuildDir.bat
rm -f cd_SrcDir.bat cd_BuildDir.bat %XSRC%/cd_BuildDir.bat
goto TestsuitFixed
:NoDirChange
call djgpp\edtest.bat
::TestsuitFixed
:touch %XSRC%/stamp-djgppfixes

Rem /include/wchar.h from DJGPP 2.03 does not work.
Rem Replace it with the one of DJGPP 2.04.
test -f %XSRC%/srclib/wchar.h
if errorlevel 1 update %XSRC%/djgpp/wchar.h %XSRC%/srclib/wchar.h

Rem This is required because DOS/Windows are case-insensitive
Rem to file names, and "make install" will do nothing if Make
Rem finds a file called `install'.
if exist INSTALL mv -f INSTALL INSTALL.txt

Rem Set SHELL to a sane default or some configure tests stop working
Rem if the package is configured across partitions.
if not "%SHELL%" == "" goto HomeName
set SHELL=/bin/sh
if not "%SHELL%" == "/bin/sh" goto SmallEnv
echo No SHELL found in the environment, using default value

:HomeName
Rem Set HOME to a sane default so configure stops complaining.
if not "%HOME%" == "" goto HostName
set HOME=%XSRC%/djgpp
if not "%HOME%" == "%XSRC%/djgpp" goto SmallEnv
echo No HOME found in the environment, using default value

:HostName
Rem Set HOSTNAME so it shows in config.status
if not "%HOSTNAME%" == "" goto hostdone
if "%windir%" == "" goto msdos
set OS=MS-Windows
if not "%OS%" == "MS-Windows" goto SmallEnv
goto haveos
:msdos
set OS=MS-DOS
if not "%OS%" == "MS-DOS" goto SmallEnv
:haveos
if not "%USERNAME%" == "" goto haveuname
if not "%USER%" == "" goto haveuser
echo No USERNAME and no USER found in the environment, using default values
set HOSTNAME=Unknown PC
if not "%HOSTNAME%" == "Unknown PC" goto SmallEnv
goto userdone
:haveuser
set HOSTNAME=%USER%'s PC
if not "%HOSTNAME%" == "%USER%'s PC" goto SmallEnv
goto userdone
:haveuname
set HOSTNAME=%USERNAME%'s PC
if not "%HOSTNAME%" == "%USERNAME%'s PC" goto SmallEnv
:userdone
set _HOSTNAME=%HOSTNAME%, %OS%
if not "%_HOSTNAME%" == "%HOSTNAME%, %OS%" goto SmallEnv
set HOSTNAME=%_HOSTNAME%
:hostdone
set _HOSTNAME=
set OS=

Rem install-sh is required by the configure script but clashes with the
Rem various Makefile install-foo targets, so we MUST have it before the
Rem script runs and rename it afterwards
test -f %XSRC%/install-sh
if not errorlevel 1 goto NoRen0
test -f %XSRC%/install-sh.sh
if not errorlevel 1 mv -f %XSRC%/install-sh.sh %XSRC%/install-sh
:NoRen0

if "%NLS%" == "disabled" goto WithoutNLS

:WithNLS
test -d %XSRC%/po
if errorlevel 1 goto WithoutNLS

Rem Check for the needed libraries and binaries.
test -x /dev/env/DJDIR/bin/msgfmt.exe
if not errorlevel 0 goto MissingNLSTools
test -x /dev/env/DJDIR/bin/xgettext.exe
if not errorlevel 0 goto MissingNLSTools
test -f /dev/env/DJDIR/include/libcharset.h
if not errorlevel 0 goto MissingNLSTools
test -f /dev/env/DJDIR/lib/libcharset.a
if not errorlevel 0 goto MissingNLSTools
test -f /dev/env/DJDIR/include/iconv.h
if not errorlevel 0 goto MissingNLSTools
test -f /dev/env/DJDIR/lib/libiconv.a
if not errorlevel 0 goto MissingNLSTools
test -f /dev/env/DJDIR/include/libintl.h
if not errorlevel 0 goto MissingNLSTools
test -f /dev/env/DJDIR/lib/libintl.a
if not errorlevel 0 goto MissingNLSTools

Rem Recreate the files in the %XSRC%/po subdir with our ported tools.
redir -e /dev/null rm %XSRC%/po/*.gmo
redir -e /dev/null rm %XSRC%/po/libiconv.pot
redir -e /dev/null rm %XSRC%/po/cat-id-tbl.c
redir -e /dev/null rm %XSRC%/po/stamp-cat-id

Rem Update the arguments file for the configure script.
Rem We prefer without-included-gettext because libintl.a from gettext package
Rem is the only one that is garanteed to have been ported to DJGPP.
echo --enable-nls --without-included-gettext >> arguments
goto ConfigurePackage

:MissingNLSTools
echo Needed libs/tools for NLS not found. Configuring without NLS.
:WithoutNLS
Rem Update the arguments file for the configure script.
echo --disable-nls >> arguments

:ConfigurePackage
echo Running the ./configure script...
sh ./configure @arguments
if errorlevel 1 goto CfgError
rm arguments
echo Done.
goto End

:SedError
echo ./configure script editing failed!
goto End

:CfgError
echo ./configure script exited abnormally!
goto End

:SmallEnv
echo Your environment size is too small.  Enlarge it and run me again.
echo Configuration NOT done!

:End
test -f %XSRC%/install-sh.sh
if not errorlevel 1 goto NoRen1
test -f %XSRC%/install-sh
if not errorlevel 1 mv -f %XSRC%/install-sh %XSRC%/install-sh.sh
:NoRen1
set CONFIG_SITE=
set HOSTNAME=
set XSRC=
