AC_DEFUN([_MAC_SAVE], [
	for mac_i in $1; do
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
	], [$X_PRE_LIBS $X_LIBS -lX11 $X_EXTRA_LIBS])
	X_CPPFLAGS="$X_CFLAGS"
	X_CFLAGS=
	_MAC_SAVE([CPPFLAGS], [
		CPPFLAGS="$CPPFLAGS $X_CPPFLAGS"
		AC_CHECK_HEADERS_ONCE([X11/Xlib.h])
	])
])

AC_DEFUN([MAC_PKG_XSCREENSAVER], [
	AC_REQUIRE([MAC_PKG_X])dnl

	HAVE_PKG_XSCREENSAVER=no
	PKG_XSCREENSAVER_CONFIGURABLE=no
	PKG_XSCREENSAVER_MODULAR=no
	PKG_XSCREENSAVER_DEFAULTDIR=
	PKG_XSCREENSAVER_HACKDIR=
	PKG_XSCREENSAVER_CONFIGDIR=
	PKG_XSCREENSAVER_MODULEDIR=

	AC_ARG_WITH(xscreensaver,
		AC_HELP_STRING([--without-xscreensaver], [do not integrate with XScreenSaver]))
	case "$with_xscreensaver" in
	yes)
		with_xscreensaver=force
		;;
	no)
		;;
	*)
		with_xscreensaver=yes
		;;
	esac

	if test "$with_xscreensaver" != no; then
		AC_ARG_WITH(hackdir,
			AC_HELP_STRING([--with-hackdir=DIR],
				[where XScreenSaver hacks are installed [[guessed]]]))
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

		AC_ARG_WITH(moduledir,
			AC_HELP_STRING([--with-moduledir=DIR],
				[where XScreenSaver modules are stored [[guessed]]]))
		case "$with_moduledir" in
			yes)
				mac_xss_use_moduledir=force
				;;
			no)
				if test "$with_xscreensaver_updater" != no; then
					AC_MSG_ERROR([*** --without-moduledir and --with-xscreensaver-updater are incompatible ***])
				fi
				mac_xss_use_moduledir=no
				;;
			"")
				mac_xss_use_moduledir=yes
				;;
			*)
				mac_xss_use_moduledir=yes
				PKG_XSCREENSAVER_MODULEDIR="$with_moduledir"
				;;
		esac

		AC_ARG_WITH(xscreensaver_updater,
			AC_HELP_STRING([--with-xscreensaver-updater=PROGRAM],
				[utility to update XScreenSaver modules [[guessed]]]))
		case "$with_xscreensaver_updater" in
		yes)
			mac_xss_updater=force
			;;
		no)
			if test "$with_moduledir" != no; then
				AC_MSG_ERROR([*** --with-moduledir and --without-xscreensaver-updater are incompatible ***])
			fi
			mac_xss_updater=no
			;;
		"")
			mac_xss_updater=yes
			;;
		*)
			mac_xss_updater=yes
			PKG_XSCREENSAVER_UPDATER="$with_xscreensaver_updater"
			;;
		esac

		AC_ARG_WITH(defaultdir,
			AC_HELP_STRING([--with-defaultdir=DIR],
				[where application defaults files are stored [[guessed]]]))
		case "$with_defaultdir" in
			yes)
				mac_xss_use_defaultdir=force
				;;
			no)
				mac_xss_use_defaultdir=no
				;;
			"")
				mac_xss_use_defaultdir=yes
				;;
			*)
				mac_xss_use_defaultdir=yes
				PKG_XSCREENSAVER_DEFAULTDIR="$with_defaultdir"
				;;
		esac

		AC_ARG_WITH(configdir,
			AC_HELP_STRING([--with-configdir=DIR],
				[where XScreenSaver config files are installed [[guessed]]]))
		case "$with_configdir" in
			yes)
				mac_xss_use_configdir=force
				;;
			no)
				mac_xss_use_configdir=no
				;;
			"")
				mac_xss_use_configdir=yes
				;;
			*)
				mac_xss_use_configdir=yes
				PKG_XSCREENSAVER_CONFIGDIR="$with_configdir"
				;;
		esac
	fi

	if test "$with_xscreensaver" != no; then
		AC_MSG_CHECKING([for XScreenSaver hacks directory])
		if test -z "$PKG_XSCREENSAVER_HACKDIR"; then
			AC_CACHE_VAL(mac_cv_xss_hackdir, [
				mac_cv_xss_hackdir=
				if test -d "$x_libraries/xscreensaver"; then
					mac_cv_xss_hackdir="$x_libraries/xscreensaver"
				elif test -d /usr/libexec/xscreensaver; then
					mac_cv_xss_hackdir=/usr/libexec/xscreensaver
				fi
			])
			if test -z "$mac_cv_xss_hackdir"; then
				AC_MSG_RESULT([not found])
				if test "$with_xscreensaver" = force; then
					AC_MSG_WARN([*** --with-xscreensaver specified but no hacks directory found ***])
					AC_MSG_ERROR([*** (try using the --with-hackdir=DIR option) ***])
				fi
				with_xscreensaver=no
			else
				PKG_XSCREENSAVER_HACKDIR="$mac_cv_xss_hackdir"
				AC_MSG_RESULT($PKG_XSCREENSAVER_HACKDIR)
			fi
		else
			AC_MSG_RESULT($PKG_XSCREENSAVER_HACKDIR)
		fi
	else
		PKG_XSCREENSAVER_HACKDIR="$bindir"
	fi

	if test "$with_xscreensaver" != no; then
		AC_MSG_CHECKING([for XScreenSaver modules directory])
		if test -z "$PKG_XSCREENSAVER_MODULEDIR"; then
			if test "$mac_xss_use_moduledir" != no; then
				AC_CACHE_VAL(mac_cv_xss_moduledir, [
					mac_cv_xss_moduledir=
					if test -d /usr/share/xscreensaver/hacks.conf.d; then
						mac_cv_xss_moduledir=/usr/share/xscreensaver/hacks.conf.d
					fi
				])
				if test -z "$mac_cv_xss_moduledir"; then
					AC_MSG_RESULT([not found])
					if test "$mac_xss_use_moduledir" = force; then
						AC_MSG_WARN([*** --with-moduledir specified but no modules directory found ***])
						AC_MSG_ERROR([*** (try specifying a directory with the --with-moduledir=DIR option) ***])
					fi
					AC_MSG_NOTICE([*** assuming non-modular XScreenSaver ***])
					AC_MSG_NOTICE([*** (if this is wrong, try using the --with-moduledir=DIR option) ***])
					mac_xss_use_moduledir=no
					mac_xss_updater=no
				else
					PKG_XSCREENSAVER_MODULEDIR="$mac_cv_xss_moduledir"
					AC_MSG_RESULT($PKG_XSCREENSAVER_MODULEDIR)
					mac_xss_use_defaultdir=no
				fi
			else
				AC_MSG_RESULT(disabled)
			fi
		else
			AC_MSG_RESULT($PKG_XSCREENSAVER_MODULEDIR)
			mac_xss_use_defaultdir=no
		fi
	fi

	if test "$with_xscreensaver" != no; then
		AC_MSG_CHECKING([for XScreenSaver update utility])
		if test -z "$PKG_XSCREENSAVER_UPDATER"; then
			if test "$mac_xss_updater" != no; then
				AC_CACHE_VAL(mac_cv_xss_updater, [
					mac_cv_xss_updater=
					if test -x /usr/sbin/update-xscreensaver-hacks; then
						mac_cv_xss_updater=/usr/sbin/update-xscreensaver-hacks
					fi
				])
				if test -z "$mac_cv_xss_updater"; then
					AC_MSG_RESULT([not found])
					if test "$mac_xss_updater" = force; then
						AC_MSG_WARN([*** --with-xscreensaver-updater specified but no update utility found ***])
						AC_MSG_ERROR([*** (try specifying a file with the --with-xscreensaver-updater=FILE option) ***])
					fi
					AC_MSG_NOTICE([*** falling back to non-modular XScreenSaver ***])
					AC_MSG_NOTICE([*** (if this is wrong, try using the --with-xscreensaver-updater=DIR option) ***])
					mac_xss_use_moduledir=no
					mac_xss_updater=no
				else
					PKG_XSCREENSAVER_UPDATER="$mac_cv_xss_updater"
					AC_MSG_RESULT($PKG_XSCREENSAVER_UPDATER)
				fi
			else
				AC_MSG_RESULT(disabled)
			fi
		else
			AC_MSG_RESULT($PKG_XSCREENSAVER_UPDATER)
		fi
	fi

	if test "$with_xscreensaver" != no; then
		AC_MSG_CHECKING([for X11 application defaults directory])
		if test -z "$PKG_XSCREENSAVER_DEFAULTDIR"; then
			if test "$mac_xss_use_defaultdir" != no; then
				AC_CACHE_VAL(mac_cv_xss_defaultdir, [
					mac_cv_xss_defaultdir=
					if test -d "$x_libraries/X11/app-defaults"; then
						mac_cv_xss_defaultdir="$x_libraries/X11/app-defaults"
					elif test -d /usr/share/X11/app-defaults; then
						mac_cv_xss_defaultdir=/usr/share/X11/app-defaults
					fi
				])
				if test -z "$mac_cv_xss_defaultdir"; then
					AC_MSG_RESULT([not found])
					if test "$mac_xss_use_defaultdir" = force; then
						AC_MSG_WARN([*** --with-defaultdir specified but no defaults directory found ***])
						AC_MSG_ERROR([*** (try specifying a directory with the --with-defaultdir=DIR option) ***])
					elif test "$with_xscreensaver" = force; then
						AC_MSG_WARN([*** --with-xscreensaver specified but no defaults directory found ***])
						AC_MSG_ERROR([*** (try using the --with-defaultdir=DIR option) ***])
					fi
					with_xscreensaver=no
				else
					PKG_XSCREENSAVER_DEFAULTDIR="$mac_cv_xss_defaultdir"
					AC_MSG_RESULT($PKG_XSCREENSAVER_DEFAULTDIR)
					mac_xss_use_moduledir=no
				fi
			else
				AC_MSG_RESULT(disabled)
			fi
		else
			AC_MSG_RESULT($PKG_XSCREENSAVER_DEFAULTDIR)
			mac_xss_use_moduledir=no
		fi
	fi

	if test "$with_xscreensaver" != no; then
		AC_MSG_CHECKING([for XScreenSaver config directory])
		if test -z "$PKG_XSCREENSAVER_CONFIGDIR"; then
			if test "$mac_xss_use_configdir" = yes; then
				AC_CACHE_VAL(mac_cv_xss_configdir, [
					mac_cv_xss_configdir=
					if test -d "$PKG_XSCREENSAVER_HACKDIR/config"; then
						mac_cv_xss_configdir="$PKG_XSCREENSAVER_HACKDIR/config"
					elif test -d /usr/share/control-center/screensavers; then
						mac_cv_xss_configdir=/usr/share/control-center/screensavers
					elif test -d /usr/share/xscreensaver/config; then
						mac_cv_xss_configdir=/usr/share/xscreensaver/config
					fi
				])
				if test -z "$mac_cv_xss_configdir"; then
					AC_MSG_RESULT([not found])
					if test "$mac_xss_use_configdir" = force; then
						AC_MSG_WARN([*** --with-configdir specified but no config directory found ***])
						AC_MSG_ERROR([*** (try specifying a directory with the --with-configdir=DIR option) ***])
					fi
					AC_MSG_NOTICE([*** assuming pre-4.0 version of XScreenSaver ***])
					AC_MSG_NOTICE([*** (if this is wrong, try using the --with-configdir=DIR option) ***])
					mac_xss_use_configdir=no
				else
					PKG_XSCREENSAVER_CONFIGDIR="$mac_cv_xss_configdir"
					AC_MSG_RESULT($mac_cv_xss_configdir)
				fi
			else
				AC_MSG_RESULT(disabled)
			fi
		else
			AC_MSG_RESULT($PKG_XSCREENSAVER_CONFIGDIR)
		fi
	fi

	if test "$with_xscreensaver" != no; then
		HAVE_PKG_XSCREENSAVER=yes
		if test "$mac_xss_use_configdir" = yes; then
			PKG_XSCREENSAVER_CONFIGURABLE=yes
		fi
		if test "$mac_xss_use_moduledir" = yes; then
			PKG_XSCREENSAVER_MODULAR=yes
		fi
	fi
])

AC_DEFUN([MAC_PKG_OPENGL], [
	AC_REQUIRE([MAC_PKG_X])

	_MAC_SAVE([LIBS], [
		HAVE_PKG_OPENGL=yes
		AC_SEARCH_LIBS(glXChooseVisual, [MesaGL GL GLX],
			[], [HAVE_PKG_OPENGL=no], [[$X_LIBS]])
		AC_SEARCH_LIBS(gluErrorString, [MesaGLU GL GLU],
			[], [HAVE_PKG_OPENGL=no], [[$X_LIBS]])
		OPENGL_LIBS="$LIBS"
		_MAC_SAVE([CPPFLAGS], [
			CPPFLAGS="$CPPFLAGS $X_CPPFLAGS"
			AC_CHECK_HEADERS_ONCE([GL/gl.h])
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
		AC_HELP_STRING([--with-openal=OPREFIX], [prefix to libopenal [[guessed]]]))
	AC_ARG_WITH(openal-includes,
		AC_HELP_STRING([--with-openal-includes=DIR], [libopenal headers directory [[OPREFIX/include]]]))
	AC_ARG_WITH(openal-libraries,
		AC_HELP_STRING([--with-openal-libraries=DIR], [libopenal libraries directory [[OPREFIX/lib]]]))

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
				[HAVE_PKG_OPENAL=yes])
			OPENAL_LIBS="$LIBS"
		])
	fi
])

AC_DEFUN([MAC_PKG_VORBISFILE], [
	AC_ARG_WITH(vorbisfile,
		AC_HELP_STRING([--with-vorbisfile=VPREFIX], [prefix to libvorbisfile [[guessed]]]))
	AC_ARG_WITH(vorbisfile-includes,
		AC_HELP_STRING([--with-vorbisfile-includes=DIR], [libvorbisfile headers directory [[VPREFIX/include]]]))
	AC_ARG_WITH(vorbisfile-libraries,
		AC_HELP_STRING([--with-vorbisfile-libraries=DIR], [libvorbisfile libraries directory [[VPREFIX/lib]]]))

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
			])
		])
	fi
])

AC_DEFUN([MAC_PKG_PNG], [
	AC_ARG_WITH(png,
		AC_HELP_STRING([--with-png=PPREFIX], [prefix to libpng [[guessed]]]))
	AC_ARG_WITH(png-includes,
		AC_HELP_STRING([--with-png-includes=DIR], [libpng headers directory [[PPREFIX/include]]]))
	AC_ARG_WITH(png-libraries,
		AC_HELP_STRING([--with-png-libraries=DIR], [libpng libraries directory [[PPREFIX/lib]]]))

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
				])
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
				], [mac_cv_libpng_supports_setjmp=unknown])
				LIBPNG_SUPPORTS_SETJMP=$mac_cv_libpng_supports_setjmp
			])
		fi
	fi
])

AC_DEFUN([_MAC_HACK_ENABLE], [
	AC_ARG_ENABLE([$1],
		AS_HELP_STRING([--disable-[$1]], [do not build $2 screensaver]))
	AS_VAR_PUSHDEF([enable], [enable_$1])dnl
	AS_VAR_PUSHDEF([note], [note_$1])dnl
	AS_VAR_SET([note], [])
	if test "AS_VAR_GET(enable)" != no; then
		AS_VAR_SET([enable], [yes])
		$3
	fi
	AM_CONDITIONAL(BUILD_$4, test "AS_VAR_GET(enable)" = yes)
	if test "AS_VAR_GET(enable)" = yes; then
		ENABLED_HACKS="$ENABLED_HACKS [$1]"
		ENABLED_BINARIES="$ENABLED_BINARIES rs-[$1]$EXEEXT"
		ENABLED_CONFIGS="$ENABLED_CONFIGS rs-[$1].xml"
		ENABLED_MODULES="$ENABLED_MODULES rs-[$1].conf"
	else
		DISABLED_HACKS="$DISABLED_HACKS [$1]"
	fi
	if test -z "AS_VAR_GET([note])"; then :; else
		AS_VAR_SET([enable], ["AS_VAR_GET(enable) (AS_VAR_GET(note))"])
	fi
	AS_VAR_POPDEF([note])dnl
	AS_VAR_POPDEF([enable])dnl
])
AC_DEFUN([MAC_HACK_ENABLE], [_MAC_HACK_ENABLE([$1],[$2],[$3],AS_TR_CPP([$1]))])

dnl Cut-down version of AC_LIB_LTDL. We don't want the option to install
dnl libltdl, or to use dlpreopen (we don't want libtool at all).
dnl Also, we can use gnulib's *_ONCE macros.
AC_DEFUN([MAC_LIB_LTDL], [
	AC_REQUIRE([AC_PROG_CC])
	AC_REQUIRE([AC_C_CONST])
	AC_REQUIRE([AC_HEADER_STDC])
	AC_REQUIRE([AC_HEADER_DIRENT])
	AC_REQUIRE([_LT_AC_CHECK_DLFCN])
	AC_REQUIRE([AC_LTDL_SHLIBEXT])
	AC_REQUIRE([AC_LTDL_SHLIBPATH])
	AC_REQUIRE([AC_LTDL_SYSSEARCHPATH])
	AC_REQUIRE([AC_LTDL_OBJDIR])
	AC_REQUIRE([AC_LTDL_DLLIB])
	AC_REQUIRE([AC_LTDL_SYMBOL_USCORE])
	AC_REQUIRE([AC_LTDL_DLSYM_USCORE])
	AC_REQUIRE([AC_LTDL_SYS_DLOPEN_DEPLIBS])
	AC_REQUIRE([AC_LTDL_FUNC_ARGZ])

	AC_CHECK_HEADERS_ONCE([assert.h ctype.h errno.h malloc.h memory.h stdlib.h stdio.h unistd.h])
	AC_CHECK_HEADERS_ONCE([dl.h sys/dl.h dld.h mach-o/dyld.h])
	AC_CHECK_HEADERS_ONCE([string.h strings.h], [break])
	AC_CHECK_FUNCS_ONCE([strchr index], [break])
	AC_CHECK_FUNCS_ONCE([strrchr rindex], [break])
	AC_CHECK_FUNCS_ONCE([memcpy bcopy], [break])
	AC_CHECK_FUNCS_ONCE([memmove strcmp])
	AC_CHECK_FUNCS_ONCE([closedir opendir readdir])
])dnl
