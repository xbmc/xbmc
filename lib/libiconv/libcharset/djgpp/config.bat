@echo off
echo Configuring GNU libcharset for DJGPP v2.x...
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
set _ARGS=#%ARGS%#%1#
if not "%_ARGS%" == "#%ARGS%#%1#" goto SmallEnv
echo %_ARGS% | grep -q "###"
if errorlevel 1 set ARGS=%ARGS% %1
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

:InPlace
Rem Update configuration files
echo Updating configuration scripts...
test -f ./configure.org
if errorlevel 1 update ./configure ./configure.org
sed -f %XSRC%/djgpp/config.sed ./configure.org > configure
if errorlevel 1 goto SedError

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
test -f %XSRC%/include/libcharset.h-in
if not errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/libcharset.h-in %XSRC%/include/libcharset.h-in
test -f %XSRC%/include/libcharset.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/libcharset.h %XSRC%/include/libcharset.h-in
test -f %XSRC%/include/libcharset.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/libcharseth.in %XSRC%/include/libcharset.h-in
test -f %XSRC%/include/libcharset.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/libcharset_h.in %XSRC%/include/libcharset.h-in
test -f %XSRC%/include/localcharset.h-in
if not errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/localcharset.h-in %XSRC%/include/localcharset.h-in
test -f %XSRC%/include/localcharset.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/localcharset.h %XSRC%/include/localcharset.h-in
test -f %XSRC%/include/localcharset.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/localcharseth.in %XSRC%/include/localcharset.h-in
test -f %XSRC%/include/localcharset.h-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/localcharset_h.in %XSRC%/include/localcharset.h-in
test -f %XSRC%/include/localcharset.h.build.in
if not errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/localcharset.h.build.in %XSRC%/include/localcharset.h-build-in
test -f %XSRC%/include/localcharset.h-build-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/localcharset.h %XSRC%/include/localcharset.h-build-in
test -f %XSRC%/include/localcharset.h-build-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/localcharseth.build %XSRC%/include/localcharset.h-build-in
test -f %XSRC%/include/localcharset.h-build-in
if errorlevel 1 redir -e /dev/null mv -f %XSRC%/include/localcharset_h.build %XSRC%/include/localcharset.h-build-in

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
redir -e /dev/null rm %XSRC%/po/libcharset.pot
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
