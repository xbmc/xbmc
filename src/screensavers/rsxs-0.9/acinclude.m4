AC_DEFUN([_MAC_SAVE], [
	for mac_i in [$1]; do
		eval "mac_save_$mac_i=\$$mac_i"
	done
$2
	for mac_i in $1; do
		eval "$mac_i=\$mac_save_$mac_i"
	done
])

dnl Based on AC_CXX_BOOL ver 1.2 by Todd Veldhuizen and Luc Maisonobe
AC_DEFUN([MAC_CXX_BOOL], [
	AC_CACHE_CHECK(
		[whether the compiler recognizes bool as a built-in type],
		mac_cv_cxx_bool, [
			AC_LANG_PUSH(C++)dnl
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
int f(int  x){return 1;}
int f(char x){return 1;}
int f(bool x){return 1;}
			], [
bool b = true; return f(b);
			])], [mac_cv_cxx_bool=yes], [mac_cv_cxx_bool=no])
			AC_LANG_POP(C++)dnl
		]
	)
	HAVE_CXX_BOOL=$mac_cv_cxx_bool
	if test "$HAVE_CXX_BOOL" = yes; then
  	AC_DEFINE(HAVE_CXX_BOOL, 1, [Define to 1 if bool is a built-in type.])dnl
	fi
])

dnl Based on AC_CXX_NEW_FOR_SCOPING ver 1.2 by Todd Veldhuizen and Luc Maisonobe
AC_DEFUN([MAC_CXX_NEW_FOR_SCOPING], [
	AC_CACHE_CHECK([whether the compiler accepts the new for scoping rules],
		mac_cv_cxx_new_for_scoping, [
			AC_LANG_PUSH(C++)dnl
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [
int z = 0;
for (int i = 0; i < 10; ++i)
	z = z + i;
for (int i = 0; i < 10; ++i)
	z = z - i;
return z;
			])], [mac_cv_cxx_new_for_scoping=yes], [mac_cv_cxx_new_for_scoping=no])
			AC_LANG_POP(C++)dnl
		]
	)
	HAVE_CXX_NEW_FOR_SCOPING=$mac_cv_cxx_new_for_scoping
	if test "$HAVE_CXX_NEW_FOR_SCOPING" = yes; then
		AC_DEFINE(HAVE_CXX_NEW_FOR_SCOPING, 1,
			[Define to 1 if the compiler accepts the new for scoping rules.])dnl
	fi
])

dnl Based on AC_CXX_NAMESPACES ver 1.2 by Todd Veldhuizen and Luc Maisonobe
AC_DEFUN([MAC_CXX_NAMESPACES], [
	AC_CACHE_CHECK([whether the compiler implements namespaces],
		mac_cv_cxx_namespaces, [
			AC_LANG_PUSH(C++)dnl
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
namespace Outer { namespace Inner { int i = 0; }}
			], [
using namespace Outer::Inner; return i;
			])], [mac_cv_cxx_namespaces=yes], [mac_cv_cxx_namespaces=no])
			AC_LANG_POP(C++)dnl
		]
	)
	HAVE_CXX_NAMESPACES=$mac_cv_cxx_namespaces
	if test "$HAVE_CXX_NAMESPACES" = yes; then
  	AC_DEFINE(HAVE_CXX_NAMESPACES, 1,
			[Define to 1 if the compiler implements namespaces.])dnl
	fi
])

dnl Based on AC_CXX_HAVE_STD ver 1.2 by Todd Veldhuizen and Luc Maisonobe
AC_DEFUN([MAC_CXX_STD], [
	AC_REQUIRE([MAC_CXX_NAMESPACES])dnl
	AC_CACHE_CHECK([whether the compiler supports ISO C++ standard library],
		mac_cv_cxx_std, [
			AC_LANG_PUSH(C++)dnl
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
@%:@include <iostream>
@%:@include <map>
@%:@include <iomanip>
@%:@include <cmath>
@%:@ifdef HAVE_CXX_NAMESPACES
using namespace std;
@%:@endif
			], [return 0;])], [mac_cv_cxx_std=yes], [mac_cv_cxx_std=no])
			AC_LANG_POP(C++)dnl
		]
	)
	HAVE_CXX_STD=$mac_cv_cxx_std
	if test "$HAVE_CXX_STD" = yes; then
  	AC_DEFINE(HAVE_CXX_STD, 1,
			[Define to 1 if the compiler supports ISO C++ standard library.])dnl
	fi
])

AC_DEFUN([MAC_CXX_SSTREAM], [
	AC_REQUIRE([MAC_CXX_STD])dnl
	AC_CACHE_CHECK([whether the C++ standard library uses stringstream or strstream],
		mac_cv_cxx_sstream, [
			AC_LANG_PUSH(C++)dnl
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
@%:@include <sstream>
@%:@ifdef HAVE_CXX_NAMESPACES
using namespace std;
@%:@endif
			], [
istringstream iss("");
ostringstream oss();
stringstream   ss();
			])], [mac_cv_cxx_sstream=stringstream], [
				AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
@%:@include <strstream>
@%:@ifdef HAVE_CXX_NAMESPACES
using namespace std;
@%:@endif
				], [
istrstream iss("");
ostrstream oss();
strstream   ss();
				])], [mac_cv_cxx_sstream=strstream], [
					mac_cv_cxx_sstream=unknown
				])
			])
			AC_LANG_POP(C++)dnl
		]
	)
	case "$mac_cv_cxx_sstream" in
	stringstream)
  	AC_DEFINE(HAVE_CXX_STRINGSTREAM, 1,
			[Define to 1 if the C++ standard library uses stringstream.])dnl
		;;
	strstream)
  	AC_DEFINE(HAVE_CXX_STRSTREAM, 1,
			[Define to 1 if the C++ standard library uses strstream.])dnl
		;;
	*)
		AC_MSG_ERROR([can't compile without either stringstream or strstream])
		;;
	esac
])

AC_DEFUN([MAC_PKG_X], [
	AC_REQUIRE([AC_PATH_XTRA])dnl
	if test "$no_x" = yes; then
		AC_MSG_ERROR([can't compile without X])
	fi
	AC_CHECK_LIB(Xmu, XmuLookupStandardColormap, [
		X_LIBS="$X_PRE_LIBS $X_LIBS -lX11 -lXmu $X_EXTRA_LIBS"
	], [
		X_LIBS="$X_PRE_LIBS $X_LIBS -lX11 $X_EXTRA_LIBS"
	], [$X_PRE_LIBS $X_LIBS -lX11 $X_EXTRA_LIBS])dnl
	X_CPPFLAGS="$X_CFLAGS"
	X_CFLAGS=
	_MAC_SAVE([CPPFLAGS], [
		CPPFLAGS="$CPPFLAGS $X_CPPFLAGS"
		AC_CHECK_HEADERS([X11/Xlib.h], , , [#])
	])
])

AC_DEFUN([MAC_PKG_XSCREENSAVER], [
	AC_REQUIRE([MAC_PKG_X])dnl

	PKG_XSCREENSAVER_SYMLINKS=yes

	AC_ARG_WITH(xscreensaver,
		AC_HELP_STRING([--without-xscreensaver], [do not integrate with XScreenSaver]))dnl
	case "$with_xscreensaver" in
	nosymlinks)
		with_xscreensaver=
		PKG_XSCREENSAVER_SYMLINKS=no
		;;
	yes)
		with_xscreensaver=force
		;;
	no)
		;;
	*)
		with_xscreensaver=yes
		;;
	esac

	HAVE_PKG_XSCREENSAVER=no
	HAVE_PKG_XSCREENSAVER_4=no
	if test "$with_xscreensaver" != no; then
		PKG_XSCREENSAVER_DEFAULTDIR=
		AC_ARG_WITH(defaultdir,
			AC_HELP_STRING([--with-defaultdir],
				[where application defaults files are stored [[guessed]]]))dnl
		case "$with_defaultdir" in
			no)
				AC_MSG_ERROR([can't install as XScreenSaver hacks without a defaultdir])
				;;
			yes | "")
				;;
			*)
				PKG_XSCREENSAVER_DEFAULTDIR="$with_defaultdir"
				;;
		esac

		PKG_XSCREENSAVER_HACKDIR=
		AC_ARG_WITH(hackdir,
			AC_HELP_STRING([--with-hackdir],
				[where XScreenSaver hacks are installed [[guessed]]]))dnl
		case "$with_hackdir" in
			no)
				AC_MSG_ERROR([can't install as XScreenSaver hacks without a hackdir])
				;;
			yes | "")
				;;
			*)
				PKG_XSCREENSAVER_HACKDIR="$with_hackdir"
				;;
		esac

		PKG_XSCREENSAVER_CONFIGDIR=
		mac_xss_use_configdir=yes
		AC_ARG_WITH(configdir,
			AC_HELP_STRING([--with-configdir],
				[where XScreensaver config files are installed [[guessed]]]))dnl
		case "$with_configdir" in
			no)
				mac_xss_use_configdir=no
				;;
			no | yes | "")
				;;
			*)
				PKG_XSCREENSAVER_CONFIGDIR="$with_configdir"
				;;
		esac
	fi

	if test "$with_xscreensaver" != no; then
		AC_MSG_CHECKING([for X11 application defaults directory])
		if test -z "$PKG_XSCREENSAVER_DEFAULTDIR"; then
			AC_CACHE_VAL(mac_cv_xss_defaultdir, [
				if test -d "$x_libraries/X11/app-defaults"; then
					mac_cv_xss_defaultdir="$x_libraries/X11/app-defaults"
				elif test -d /usr/share/X11/app-defaults; then
					mac_cv_xss_defaultdir=/usr/share/X11/app-defaults
				else
					mac_cv_xss_defaultdir=
				fi
			])
			if test -z "$mac_cv_xss_defaultdir"; then
				AC_MSG_RESULT([not found])
				if test "$with_xscreensaver" = force; then
					AC_MSG_WARN([*** XScreenSaver support will be disabled ***])
					AC_MSG_WARN([*** (try using the --with-defaultdir=DIR option) ***])
					with_xscreensaver=no
				else
					AC_MSG_ERROR([try using the --with-defaultdir=DIR option])
				fi
			else
				PKG_XSCREENSAVER_DEFAULTDIR="$mac_cv_xss_defaultdir"
				AC_MSG_RESULT($PKG_XSCREENSAVER_DEFAULTDIR)
			fi
		else
			AC_MSG_RESULT($PKG_XSCREENSAVER_DEFAULTDIR)
		fi
	fi

	if test "$with_xscreensaver" != no; then
		AC_MSG_CHECKING([for XScreenSaver hacks directory])
		if test -z "$PKG_XSCREENSAVER_HACKDIR"; then
			AC_CACHE_VAL(mac_cv_xss_hackdir, [
				if test -d "$x_libraries/xscreensaver"; then
					mac_cv_xss_hackdir="$x_libraries/xscreensaver"
				elif test -d /usr/libexec/xscreensaver; then
					mac_cv_xss_hackdir=/usr/libexec/xscreensaver
				else
					mac_cv_xss_hackdir=
				fi
			])dnl
			if test -z "$mac_cv_xss_hackdir"; then
				AC_MSG_RESULT([not found])
				if test "$with_xscreensaver" = force; then
					AC_MSG_WARN([*** XScreenSaver support will be disabled ***])
					AC_MSG_WARN([*** (try using the --with-hackdir=DIR option) ***])
					with_xscreensaver=no
				else
					AC_MSG_ERROR([try using the --with-hackdir=DIR option])
				fi
			else
				PKG_XSCREENSAVER_HACKDIR="$mac_cv_xss_hackdir"
				AC_MSG_RESULT($PKG_XSCREENSAVER_HACKDIR)
			fi
		else
			AC_MSG_RESULT($PKG_XSCREENSAVER_HACKDIR)
		fi
	fi

	if test "$with_xscreensaver" != no; then
		AC_MSG_CHECKING([for XScreenSaver config directory])
		if test -z "$PKG_XSCREENSAVER_CONFIGDIR"; then
			if test "$mac_xss_use_configdir" = yes; then
				AC_CACHE_VAL(mac_cv_xss_configdir, [
					if test -d "$PKG_XSCREENSAVER_HACKDIR/config"; then
						mac_cv_xss_configdir="$PKG_XSCREENSAVER_HACKDIR/config"
					elif test -d /usr/share/control-center/screensavers; then
						mac_cv_xss_configdir=/usr/share/control-center/screensavers
					elif test -d /usr/share/xscreensaver/config; then
						mac_cv_xss_configdir=/usr/share/xscreensaver/config
					else
						mac_cv_xss_configdir=
					fi
				])
				if test -z "$mac_cv_xss_configdir"; then
					AC_MSG_RESULT([not found])
					AC_MSG_NOTICE([*** assuming pre-4.0 version of XScreenSaver ***])
					AC_MSG_NOTICE([*** (if this is wrong, try using the --with-configdir=DIR option) ***])
					mac_xss_use_configdir=no
				else
					AC_MSG_RESULT($mac_cv_xss_configdir)
				fi
				PKG_XSCREENSAVER_CONFIGDIR="$mac_cv_xss_configdir"
			else
				AC_MSG_RESULT(disabled)
			fi
		else
			AC_MSG_RESULT($PKG_XSCREENSAVER_CONFIGDIR)
		fi
	fi

	if test "$with_xscreensaver" != no; then
		if test "$mac_xss_use_configdir" = yes; then
			HAVE_PKG_XSCREENSAVER_4=yes
		fi
		HAVE_PKG_XSCREENSAVER=yes
	else
		PKG_XSCREENSAVER_DEFAULTDIR=
		PKG_XSCREENSAVER_HACKDIR="$bindir"
		PKG_XSCREENSAVER_CONFIGDIR=
		PKG_XSCREENSAVER_SYMLINKS=no
	fi
])

AC_DEFUN([MAC_PKG_OPENGL], [
	AC_REQUIRE([MAC_PKG_X])

	_MAC_SAVE([LIBS], [
		HAVE_PKG_OPENGL=yes
		AC_SEARCH_LIBS(glXChooseVisual, [MesaGL GL GLX],
			[], [HAVE_PKG_OPENGL=no], [[$X_LIBS]])dnl
		AC_SEARCH_LIBS(gluErrorString, [MesaGLU GL GLU],
			[], [HAVE_PKG_OPENGL=no], [[$X_LIBS]])dnl
		OPENGL_LIBS="$LIBS"
		_MAC_SAVE([CPPFLAGS], [
			CPPFLAGS="$CPPFLAGS $X_CPPFLAGS"
			AC_CHECK_HEADERS([GL/gl.h], , , [#])
			AC_CHECK_HEADERS([GL/glext.h GL/glu.h GL/glx.h], , , [
@%:@if HAVE_GL_GL_H
@%:@include <GL/gl.h>
@%:@endif
			])
		])
	])
])

AC_DEFUN([MAC_PKG_OPENAL], [
	AC_ARG_WITH(openal,
		AC_HELP_STRING([--with-openal=OPREFIX], [prefix to libopenal [[guessed]]]))dnl
	AC_ARG_WITH(openal-includes,
		AC_HELP_STRING([--with-openal-includes=DIR], [libopenal headers directory [[OPREFIX/include]]]))dnl
	AC_ARG_WITH(openal-libraries,
		AC_HELP_STRING([--with-openal-libraries=DIR], [libopenal libraries directory [[OPREFIX/lib]]]))dnl

	HAVE_PKG_OPENAL=no
	if test "$with_openal" != no; then
		OPENAL_CPPFLAGS=
		if test x"$with_openal_includes" != x; then
			OPENAL_CPPFLAGS="-I$with_openal_includes"
		elif test x"$with_openal" != x; then
			OPENAL_CPPFLAGS="-I$with_openal/include"
		fi

		OPENAL_LDFLAGS=
		if test x"$with_openal_libraries" != x; then
			OPENAL_LDFLAGS="-L$with_openal_libraries"
		elif test x"$with_openal" != x; then
			OPENAL_LDFLAGS="-L$with_openal/lib"
		fi

		OPENAL_LIBS=
		_MAC_SAVE([LIBS], [
			AC_SEARCH_LIBS(alcOpenDevice, [openal AL openAL],
				[HAVE_PKG_OPENAL=yes])dnl
			OPENAL_LIBS="$LIBS"
		])
	fi
])

AC_DEFUN([MAC_PKG_VORBISFILE], [
	AC_ARG_WITH(vorbisfile,
		AC_HELP_STRING([--with-vorbisfile=VPREFIX], [prefix to libvorbisfile [[guessed]]]))dnl
	AC_ARG_WITH(vorbisfile-includes,
		AC_HELP_STRING([--with-vorbisfile-includes=DIR], [libvorbisfile headers directory [[VPREFIX/include]]]))dnl
	AC_ARG_WITH(vorbisfile-libraries,
		AC_HELP_STRING([--with-vorbisfile-libraries=DIR], [libvorbisfile libraries directory [[VPREFIX/lib]]]))dnl

	HAVE_PKG_VORBISFILE=no
	if test "$with_vorbisfile" != no; then
		VORBISFILE_CPPFLAGS=
		if test x"$with_vorbisfile_includes" != x; then
			VORBISFILE_CPPFLAGS="-I$with_vorbisfile_includes"
		elif test x"$with_vorbisfile" != x; then
			VORBISFILE_CPPFLAGS="-I$with_vorbisfile/include"
		fi

		VORBISFILE_LDFLAGS=
		if test x"$with_vorbisfile_libraries" != x; then
			VORBISFILE_LDFLAGS="-I$with_vorbisfile_libraries"
		elif test x"$with_vorbisfile" != x; then
			VORBISFILE_LDFLAGS="-I$with_vorbisfile/lib"
		fi

		VORBISFILE_LIBS=
		_MAC_SAVE([CPPFLAGS LDFLAGS LIBS], [
			CPPFLAGS="$CPPFLAGS $VORBISFILE_CPPFLAGS"
			LDFLAGS="$LDFLAGS $VORBISFILE_LDFLAGS"
			LIBS=
			AC_CHECK_LIB(vorbisfile, ov_open, [
				VORBISFILE_LIBS="-lvorbisfile"
				HAVE_PKG_VORBISFILE=yes
			])dnl
		])
	fi
])

AC_DEFUN([MAC_PKG_PNG], [
	AC_ARG_WITH(png,
		AC_HELP_STRING([--with-png=PPREFIX], [prefix to libpng [[guessed]]]))dnl
	AC_ARG_WITH(png-includes,
		AC_HELP_STRING([--with-png-includes=DIR], [libpng headers directory [[PPREFIX/include]]]))dnl
	AC_ARG_WITH(png-libraries,
		AC_HELP_STRING([--with-png-libraries=DIR], [libpng libraries directory [[PPREFIX/lib]]]))dnl

	HAVE_PKG_PNG=no
	LIBPNG_SUPPORTS_SETJMP=no
	if test "$with_png" != no; then
		PNG_CPPFLAGS=
		if test x"$with_png_includes" != x; then
			PNG_CPPFLAGS="-I$with_png_includes"
		elif test x"$with_png" != x; then
			PNG_CPPFLAGS="-I$with_png/include"
		fi

		PNG_LDFLAGS=
		if test x"$with_png_libraries" != x; then
			PNG_LDFLAGS="-L$with_png_libraries"
		elif test x"$with_png" != x; then
			PNG_LDFLAGS="-L$with_png/lib"
		fi

		PNG_LIBS=

		mac_libpng_config=libpng-config
		if test x"$with_png" != x; then
			mac_libpng_config="$with_png/bin/libpng-config"
		fi
		AC_CACHE_CHECK([for libpng-config], mac_cv_pkg_png, [
			AC_TRY_COMMAND([$mac_libpng_config --version >&AS_MESSAGE_LOG_FD 2>&AS_MESSAGE_LOG_FD])
			mac_cv_pkg_png="not found"
			if test "$ac_status" = 0; then
				mac_cv_pkg_png="$mac_libpng_config"
			fi
		])
		if test "$mac_cv_pkg_png" != "not found"; then
			AC_CACHE_CHECK([for libpng preprocessor flags], mac_cv_pkg_cppflags, [
				mac_cv_pkg_cppflags=`"$mac_libpng_config" --I_opts --cppflags 2>&AS_MESSAGE_LOG_FD`
				mac_cv_pkg_cppflags=`echo $mac_cv_pkg_cppflags`
			])
			PNG_CPPFLAGS="$mac_cv_pkg_cppflags"
			AC_CACHE_CHECK([for libpng compiler flags], mac_cv_pkg_cxxflags, [
				mac_cv_pkg_cxxflags=`"$mac_libpng_config" --ccopts 2>&AS_MESSAGE_LOG_FD`
				mac_cv_pkg_cxxflags=`echo $mac_cv_pkg_cxxflags`
			])
			PNG_CXXFLAGS="$mac_cv_pkg_cxxflags"
			AC_CACHE_CHECK([for libpng linker flags], mac_cv_pkg_ldflags, [
				mac_cv_pkg_ldflags=`"$mac_libpng_config" --L_opts --R_opts 2>&AS_MESSAGE_LOG_FD`
				mac_cv_pkg_ldflags=`echo $mac_cv_pkg_ldflags`
			])
			PNG_LDFLAGS="$mac_cv_pkg_ldflags"
			AC_CACHE_CHECK([for libpng libraries], mac_cv_pkg_libs, [
				mac_cv_pkg_libs=`"$mac_libpng_config" --libs 2>&AS_MESSAGE_LOG_FD`
				mac_cv_pkg_libs=`echo $mac_cv_pkg_libs`
			])
			PNG_LIBS="$mac_cv_pkg_libs"
			HAVE_PKG_PNG=yes
		fi

		if test "$HAVE_PKG_PNG" = no; then
			_MAC_SAVE([CPPFLAGS CXXFLAGS LDFLAGS LIBS], [
				CPPFLAGS="$CPPFLAGS $PNG_CPPFLAGS"
				CXXFLAGS="$CXXFLAGS $PNG_CXXFLAGS"
				LDFLAGS="$LDFLAGS $PNG_LDFLAGS"
				LIBS=
				AC_CHECK_LIB(png, png_access_version_number, [
					PNG_LIBS="-lpng"
					HAVE_PKG_PNG=yes
				])dnl
			])
		fi

		if test "$HAVE_PKG_PNG" = yes; then
			_MAC_SAVE([CPPFLAGS CXXFLAGS LDFLAGS LIBS], [
				CPPFLAGS="$CPPFLAGS $PNG_CPPFLAGS"
				CXXFLAGS="$CXXFLAGS $PNG_CXXFLAGS"
				LDFLAGS="$LDFLAGS $PNG_LDFLAGS"
				LIBS="$PNG_LIBS"
				AC_CHECK_HEADERS([png.h], [
					AC_CACHE_CHECK(
						[whether libpng supports setjmp],
						mac_cv_libpng_supports_setjmp, [
							AC_COMPILE_IFELSE([
@%:@include <png.h>
@%:@if !defined PNG_SETJMP_SUPPORTED
@%:@error PNG_SETJMP_SUPPORTED not defined
@%:@endif
							], [mac_cv_libpng_supports_setjmp=yes], [mac_cv_libpng_supports_setjmp=no]
							)
						]
					)
				], [mac_cv_libpng_supports_setjmp=unknown], [#])
				LIBPNG_SUPPORTS_SETJMP=$mac_cv_libpng_supports_setjmp
			])
		fi
	fi
])

AC_DEFUN([_MAC_HACK_ENABLE], [
	AC_ARG_ENABLE([$1],
		AS_HELP_STRING(--disable-[$1],do not build [$2] screensaver))dnl
	enable_hack=no
	if test "$enable_[$1]" != no; then
		enable_hack=yes
		enable_[$1]=yes
		[$3]
	fi
	AM_CONDITIONAL(BUILD_$4, test "$enable_hack" = yes)
	if test "$enable_hack" = yes; then
		ENABLED_HACKS="$ENABLED_HACKS [$1]"
		ENABLED_BINARIES="$ENABLED_BINARIES rs-[$1]$EXEEXT"
		ENABLED_CONFIGS="$ENABLED_CONFIGS rs-[$1].xml"
	else
		DISABLED_HACKS="$DISABLED_HACKS [$1]"
	fi
])
AC_DEFUN([MAC_HACK_ENABLE], [_MAC_HACK_ENABLE([$1],[$2],[$3],AS_TR_CPP([$1]))])

dnl @synopsis adl_NORMALIZE_PATH(VARNAME, [REFERENCE_STRING])
dnl
dnl Perform some cleanups on the value of $VARNAME (interpreted as a path):
dnl   - empty paths are changed to '.'
dnl   - trailing slashes are removed
dnl   - repeated slashes are squeezed except a leading doubled slash '//'
dnl     (which might indicate a networked disk on some OS).
dnl
dnl REFERENCE_STRING is used to turn '/' into '\' and vice-versa:
dnl if REFERENCE_STRING contains some backslashes, all slashes and backslashes
dnl are turned into backslashes, otherwise they are all turned into slashes.
dnl
dnl This makes processing of DOS filenames quite easier, because you
dnl can turn a filename to the Unix notation, make your processing, and
dnl turn it back to original notation.
dnl
dnl   filename='A:\FOO\\BAR\'
dnl   old_filename="$filename"
dnl   # Switch to the unix notation
dnl   adl_NORMALIZE_PATH([filename], ["/"])
dnl   # now we have $filename = 'A:/FOO/BAR' and we can process it as if
dnl   # it was a Unix path.  For instance let's say that you want
dnl   # to append '/subpath':
dnl   filename="$filename/subpath"
dnl   # finally switch back to the original notation
dnl   adl_NORMALIZE_PATH([filename], ["$old_filename"])
dnl   # now $filename equals to 'A:\FOO\BAR\subpath'
dnl
dnl One good reason to make all path processing with the unix convention
dnl is that backslashes have a special meaning in many cases.  For instance
dnl
dnl   expr 'A:\FOO' : 'A:\Foo'
dnl
dnl will return 0 because the second argument is a regex in which
dnl backslashes have to be backslashed.  In other words, to have the
dnl two strings to match you should write this instead:
dnl
dnl   expr 'A:\Foo' : 'A:\\Foo'
dnl
dnl Such behavior makes DOS filenames extremely unpleasant to work with.
dnl So temporary turn your paths to the Unix notation, and revert
dnl them to the original notation after the processing.  See the
dnl macro adl_COMPUTE_RELATIVE_PATHS for a concrete example of this.
dnl
dnl REFERENCE_STRING defaults to $VARIABLE, this means that slashes
dnl will be converted to backslashes if $VARIABLE already contains
dnl some backslashes (see $thirddir below).
dnl
dnl   firstdir='/usr/local//share'
dnl   seconddir='C:\Program Files\\'
dnl   thirddir='C:\home/usr/'
dnl   adl_NORMALIZE_PATH([firstdir])
dnl   adl_NORMALIZE_PATH([seconddir])
dnl   adl_NORMALIZE_PATH([thirddir])
dnl   # $firstdir = '/usr/local/share'
dnl   # $seconddir = 'C:\Program Files'
dnl   # $thirddir = 'C:\home\usr'
dnl
dnl @author Alexandre Duret-Lutz <duret_g@epita.fr>
dnl @version $Id: normpath.m4,v 1.1 2001/07/26 01:12 ac-archive-0.5.37 $
AC_DEFUN([adl_NORMALIZE_PATH],
[case ":[$]$1:" in
# change empty paths to '.'
  ::) $1='.' ;;
# strip trailing slashes
  :*[[\\/]]:) $1=`echo "[$]$1" | sed 's,[[\\/]]*[$],,'` ;;
  :*:) ;;
esac
# squeze repeated slashes
case ifelse($2,,"[$]$1",$2) in
# if the path contains any backslashes, turn slashes into backslashes
 *\\*) $1=`echo "[$]$1" | sed 's,\(.\)[[\\/]][[\\/]]*,\1\\\\,g'` ;;
# if the path contains slashes, also turn backslashes into slashes
 *) $1=`echo "[$]$1" | sed 's,\(.\)[[\\/]][[\\/]]*,\1/,g'` ;;
esac])

dnl @synopsis adl_COMPUTE_RELATIVE_PATHS(PATH_LIST)
dnl
dnl PATH_LIST is a space-separated list of colon-separated
dnl triplets of the form 'FROM:TO:RESULT'.  This function
dnl iterates over these triplets and set $RESULT to the
dnl relative path from $FROM to $TO.  Note that $FROM
dnl and $TO needs to be absolute filenames for this macro
dnl to success.
dnl
dnl For instance,
dnl
dnl    first=/usr/local/bin
dnl    second=/usr/local/share
dnl    adl_COMPUTE_RELATIVE_PATHS([first:second:fs second:first:sf])
dnl    # $fs is set to ../share
dnl    # $sf is set to ../bin
dnl
dnl $FROM and $TO are both eval'ed recursively and normalized,
dnl this means that you can call this macro with autoconf's dirnames
dnl like `prefix' or `datadir'.  For example:
dnl
dnl    adl_COMPUTE_RELATIVE_PATHS([bindir:datadir:bin_to_data])
dnl
dnl adl_COMPUTE_RELATIVE_PATHS should also works with DOS filenames.
dnl
dnl You may want to use this macro in order to make your package
dnl relocatable.  Instead of hardcoding $datadir into your programs
dnl just encode $bin_to_data and try to determine $bindir at run-time.
dnl
dnl This macro requires adl_NORMALIZE_PATH.
dnl
dnl @author Alexandre Duret-Lutz <duret_g@epita.fr>
dnl @version $Id: relpaths.m4,v 1.1 2001/07/26 01:12 ac-archive-0.5.37 $
AC_DEFUN([adl_COMPUTE_RELATIVE_PATHS],
[for _lcl_i in $1; do
  _lcl_from=\[$]`echo "[$]_lcl_i" | sed 's,:.*$,,'`
  _lcl_to=\[$]`echo "[$]_lcl_i" | sed 's,^[[^:]]*:,,' | sed 's,:[[^:]]*$,,'`
  _lcl_result_var=`echo "[$]_lcl_i" | sed 's,^.*:,,'`
  adl_RECURSIVE_EVAL([[$]_lcl_from], [_lcl_from])
  adl_RECURSIVE_EVAL([[$]_lcl_to], [_lcl_to])
  _lcl_notation="$_lcl_from$_lcl_to"
  adl_NORMALIZE_PATH([_lcl_from],['/'])
  adl_NORMALIZE_PATH([_lcl_to],['/'])
  adl_COMPUTE_RELATIVE_PATH([_lcl_from], [_lcl_to], [_lcl_result_tmp])
  adl_NORMALIZE_PATH([_lcl_result_tmp],["[$]_lcl_notation"])
  eval $_lcl_result_var='[$]_lcl_result_tmp'
done])

## Note:
## *****
## The following helper macros are too fragile to be used out
## of adl_COMPUTE_RELATIVE_PATHS (mainly because they assume that
## paths are normalized), that's why I'm keeping them in the same file.
## Still, some of them maybe worth to reuse.

dnl adl_COMPUTE_RELATIVE_PATH(FROM, TO, RESULT)
dnl ===========================================
dnl Compute the relative path to go from $FROM to $TO and set the value
dnl of $RESULT to that value.  This function work on raw filenames
dnl (for instead it will considerate /usr//local and /usr/local as
dnl two distinct paths), you should really use adl_COMPUTE_REALTIVE_PATHS
dnl instead to have the paths sanitized automatically.
dnl
dnl For instance:
dnl    first_dir=/somewhere/on/my/disk/bin
dnl    second_dir=/somewhere/on/another/disk/share
dnl    adl_COMPUTE_RELATIVE_PATH(first_dir, second_dir, first_to_second)
dnl will set $first_to_second to '../../../another/disk/share'.
AC_DEFUN([adl_COMPUTE_RELATIVE_PATH],
[adl_COMPUTE_COMMON_PATH([$1], [$2], [_lcl_common_prefix])
adl_COMPUTE_BACK_PATH([$1], [_lcl_common_prefix], [_lcl_first_rel])
adl_COMPUTE_SUFFIX_PATH([$2], [_lcl_common_prefix], [_lcl_second_suffix])
$3="[$]_lcl_first_rel[$]_lcl_second_suffix"])

dnl adl_COMPUTE_COMMON_PATH(LEFT, RIGHT, RESULT)
dnl ============================================
dnl Compute the common path to $LEFT and $RIGHT and set the result to $RESULT.
dnl
dnl For instance:
dnl    first_path=/somewhere/on/my/disk/bin
dnl    second_path=/somewhere/on/another/disk/share
dnl    adl_COMPUTE_COMMON_PATH(first_path, second_path, common_path)
dnl will set $common_path to '/somewhere/on'.
AC_DEFUN([adl_COMPUTE_COMMON_PATH],
[$3=''
_lcl_second_prefix_match=''
while test "[$]_lcl_second_prefix_match" != 0; do
  _lcl_first_prefix=`expr "x[$]$1" : "x\([$]$3/*[[^/]]*\)"`
  _lcl_second_prefix_match=`expr "x[$]$2" : "x[$]_lcl_first_prefix"`
  if test "[$]_lcl_second_prefix_match" != 0; then
    if test "[$]_lcl_first_prefix" != "[$]$3"; then
      $3="[$]_lcl_first_prefix"
    else
      _lcl_second_prefix_match=0
    fi
  fi
done])

dnl adl_COMPUTE_SUFFIX_PATH(PATH, SUBPATH, RESULT)
dnl ==============================================
dnl Substrack $SUBPATH from $PATH, and set the resulting suffix
dnl (or the empty string if $SUBPATH is not a subpath of $PATH)
dnl to $RESULT.
dnl
dnl For instace:
dnl    first_path=/somewhere/on/my/disk/bin
dnl    second_path=/somewhere/on
dnl    adl_COMPUTE_SUFFIX_PATH(first_path, second_path, common_path)
dnl will set $common_path to '/my/disk/bin'.
AC_DEFUN([adl_COMPUTE_SUFFIX_PATH],
[$3=`expr "x[$]$1" : "x[$]$2/*\(.*\)"`])

dnl adl_COMPUTE_BACK_PATH(PATH, SUBPATH, RESULT)
dnl ============================================
dnl Compute the relative path to go from $PATH to $SUBPATH, knowing that
dnl $SUBPATH is a subpath of $PATH (any other words, only repeated '../'
dnl should be needed to move from $PATH to $SUBPATH) and set the value
dnl of $RESULT to that value.  If $SUBPATH is not a subpath of PATH,
dnl set $RESULT to the empty string.
dnl
dnl For instance:
dnl    first_path=/somewhere/on/my/disk/bin
dnl    second_path=/somewhere/on
dnl    adl_COMPUTE_BACK_PATH(first_path, second_path, back_path)
dnl will set $back_path to '../../../'.
AC_DEFUN([adl_COMPUTE_BACK_PATH],
[adl_COMPUTE_SUFFIX_PATH([$1], [$2], [_lcl_first_suffix])
$3=''
_lcl_tmp='xxx'
while test "[$]_lcl_tmp" != ''; do
  _lcl_tmp=`expr "x[$]_lcl_first_suffix" : "x[[^/]]*/*\(.*\)"`
  if test "[$]_lcl_first_suffix" != ''; then
     _lcl_first_suffix="[$]_lcl_tmp"
     $3="../[$]$3"
  fi
done])


dnl adl_RECURSIVE_EVAL(VALUE, RESULT)
dnl =================================
dnl Interpolate the VALUE in loop until it doesn't change,
dnl and set the result to $RESULT.
dnl WARNING: It's easy to get an infinite loop with some unsane input.
AC_DEFUN([adl_RECURSIVE_EVAL],
[_lcl_receval="$1"
$2=`(test "x$prefix" = xNONE && prefix="$ac_default_prefix"
     test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
     _lcl_receval_old=''
     while test "[$]_lcl_receval_old" != "[$]_lcl_receval"; do
       _lcl_receval_old="[$]_lcl_receval"
       eval _lcl_receval="\"[$]_lcl_receval\""
     done
     echo "[$]_lcl_receval")`])
