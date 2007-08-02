dnl -------------------------------------------------------------------------
dnl Try to find a file (or one of more files in a list of dirs).
dnl $1=files
dnl $2=dirs to search
dnl $3=found dir
dnl [$4=found file]
dnl -------------------------------------------------------------------------

AC_DEFUN([MY_FIND_FILE],
[
    $3=NO
    for i in $2; do
        for j in $1; do
            if test -r "$i/$j"; then
                $3=$i
                if test "$4" != ""; then
                    $4=$j
                fi
                break 2
            fi
        done
    done
])


dnl -------------------------------------------------------------------------
AC_DEFUN([MY_SUBST_DEF],
[
    eval "$1=\"#define $1\""
    AC_SUBST($1)
])

AC_DEFUN([MY_SUBST_UNDEF],
[
    eval "$1=\"#undef $1\""
    AC_SUBST($1)
])

AC_DEFUN([MY_SUBST],
[
    eval "$1=$2"
    AC_SUBST($1)
])





dnl -------------------------------------------------------------------------
dnl Check whether compiler has a working ``bool'' type.
dnl Will substitute @HAVE_BOOL@ with either a def or undef line.
dnl -------------------------------------------------------------------------

AC_DEFUN([MY_CHECK_BOOL],
[
    AC_MSG_CHECKING([for bool])
    AC_CACHE_VAL(test_cv_have_bool,
    [
        AC_TRY_COMPILE(
            [],
            [bool aBool = true;],
            [test_cv_have_bool=yes],
            [test_cv_have_bool=no]
        )
    ])
    AC_MSG_RESULT($test_cv_have_bool)
    if test "$test_cv_have_bool" = yes; then
        AC_DEFINE(HAVE_BOOL,,[Define if the C++ compiler supports BOOL])
    fi
])

dnl -------------------------------------------------------------------------
dnl Check whether C++ library has member ios::bin instead of ios::binary.
dnl Will substitute @HAVE_IOS_BIN@ with either a def or undef line.
dnl -------------------------------------------------------------------------

AC_DEFUN([MY_CHECK_IOS_BIN],
[
    AC_MSG_CHECKING([whether standard member ios::binary is available])
    AC_CACHE_VAL(test_cv_have_ios_binary,
    [
        AC_TRY_COMPILE(
            [#include <fstream.h>],
            [ifstream myTest(ios::in|ios::binary);],
            [test_cv_have_ios_binary=yes],
            [test_cv_have_ios_binary=no]
        )
    ])
    AC_MSG_RESULT($test_cv_have_ios_binary)
    if test "$test_cv_have_ios_binary" = yes; then
        AC_DEFINE(HAVE_IOS_BIN,,
            [Define if standard member ``ios::binary'' is called ``ios::bin''.]
        )
    fi
])

dnl -------------------------------------------------------------------------
dnl Check whether C++ library has member ios::bin instead of ios::binary.
dnl Will substitute @HAVE_IOS_OPENMODE@ with either a def or undef line.
dnl -------------------------------------------------------------------------

AC_DEFUN([MY_CHECK_IOS_OPENMODE],
[
    AC_MSG_CHECKING([whether standard member ios::openmode is available])
    AC_CACHE_VAL(test_cv_have_ios_openmode,
    [
        AC_TRY_COMPILE(
            [#include <fstream.h>
             #include <iomanip.h>],
            [ios::openmode myTest = ios::in;],
            [test_cv_have_ios_openmode=yes],
            [test_cv_have_ios_openmode=no]
        )
    ])
    AC_MSG_RESULT($test_cv_have_ios_openmode)
    if test "$test_cv_have_ios_openmode" = yes; then
        AC_DEFINE(HAVE_IOS_OPENMODE,,
            [Define if ``ios::openmode'' is supported.]
        )
    fi
])

dnl -------------------------------------------------------------------------
dnl Check whether C++ environment provides the "nothrow allocator".
dnl Will substitute @HAVE_EXCEPTIONS@ if test code compiles.
dnl -------------------------------------------------------------------------

AC_DEFUN([MY_CHECK_EXCEPTIONS],
[
    AC_MSG_CHECKING([whether exceptions are available])
    AC_CACHE_VAL(test_cv_have_exceptions,
    [
        AC_TRY_COMPILE(
            [#include <new.h>],
            [char* buf = new(nothrow) char[1024];],
            [test_cv_have_exceptions=yes],
            [test_cv_have_exceptions=no]
        )
    ])
    AC_MSG_RESULT($test_cv_have_exceptions)
    if test "$test_cv_have_exceptions" = yes; then
        AC_DEFINE(HAVE_EXCEPTIONS,,
            [Define if your C++ compiler implements exception-handling.]
        )
    fi
])

dnl -------------------------------------------------------------------------
dnl Library compile test using libtool
dnl $1 = CXXFLAGS
dnl $2 = LDFLAGS
dnl $3 = include header
dnl $4 = program to compile
dnl $5 = variable name to store result
dnl -------------------------------------------------------------------------
AC_DEFUN([MY_TRY_COMPILE],
[
    my_save_cxxflags=$CXXFLAGS
    my_save_ldflags=$LDFLAGS
    my_save_cxx=$CXX

    CXXFLAGS="$CXXFLAGS $1"
    LDFLAGS="$LDFLAGS $2"
    CXX="${SHELL-/bin/sh} ${srcdir}/libtool $CXX"

    AC_TRY_LINK(
        [#include <$3>],
        [$4;],
        [$5=YES],
        [$5=NO]
    )

    CXXFLAGS=$my_save_cxxflags
    LDFLAGS=$my_save_ldflags
    CXX=$my_save_cxx
])


dnl -------------------------------------------------------------------------
dnl Expand prefix variable and others relying on it
dnl -------------------------------------------------------------------------
AC_DEFUN([MY_EXPAND_PREFIX],
[
    # Expand prefix/exec_prefix
    my_save_prefix="$prefix"
    my_save_exec_prefix="$exec_prefix"
    test "x$prefix" = xNONE && prefix=$ac_default_prefix
    test "x$exec_prefix" = xNONE && exec_prefix=$prefix
    my_def_prefix=$prefix
    my_def_exec_prefix=$exec_prefix
    eval my_def_includedir=$includedir
    eval my_def_libdir=$libdir
    # Restore old prefix
    prefix="$my_save_prefix"
    exec_prefix="$my_save_exec_prefix"
])


dnl -------------------------------------------------------------------------
dnl Find pkg-config on the system
dnl Returns path to program in variable PKG_CONFIG
dnl Modifies PKG_CONFIG_PATH to contain default install prefix
dnl $1 - Version
dnl -------------------------------------------------------------------------
AC_DEFUN([MY_CONFIG_PKG_CONFIG],
[
    dnl Find pkg-config
    AC_PATH_PROG(PKG_CONFIG, pkg-config, NO)
    if test "$PKG_CONFIG" = NO; then
        AC_MSG_ERROR([
pkg-config not found. See http://www.freedesktop.org/software/pkgconfig/
        ])
    fi

    if ! pkg-config --atleast-pkgconfig-version $1 ; then
        AC_MSG_ERROR([
pkg-config too old; version $1 or better required
        ])
    fi

    if test "$my_def_prefix" = ""; then
        MY_EXPAND_PREFIX
    fi

    # Now add libdir to the pkgconfig search path using
    # expanded prefix
    PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$my_def_libdir/pkgconfig"
    export PKG_CONFIG_PATH
    my_save_pkg_config_path="$PKG_CONFIG_PATH"
])


dnl -------------------------------------------------------------------------
dnl Find library on the system using pkgconfig.  You will automatically
dnl get LDFLAGS & CXXFLAGS using modified include and library paths if
dnl necessary.  Other variables will be obtained with the original intended
dnl install paths.  Results in uppercase output variables $library_$variable.
dnl $1 - library
dnl $2 - version
dnl $3 - variables
dnl $4 - library header
dnl $5 - program body
dnl [$6 - used header]
dnl -------------------------------------------------------------------------
AC_DEFUN([MY_FIND_PKG_CONFIG_LIB],
[
    dnl Check if pkg-config is configured
    if test "$PKG_CONFIG" = ""; then
        MY_CONFIG_PKG_CONFIG(0.5)
    fi

    AC_MSG_CHECKING([for working $1 library and headers])

    dnl Be pessimistic.
    my_includedir=NO
    my_libdir=NO
    my_uname=LIB`echo $1 | tr [[a-z]] [[A-Z]]`

    AC_ARG_WITH($1,
        [  --with-$1=DIR
            where the $1 is located],
        [my_includedir="$withval"
         my_libdir="$withval"]
    )

    AC_ARG_WITH($1-includes,
        [  --with-$1-includes=DIR
            where the $1 includes are located],
        [my_includedir="$withval"]
    )

    AC_ARG_WITH($1-library,
        [  --with-$1-library=DIR
            where the $1 library is installed],
        [my_libdir="$withval"]
    )

    # Find paths to headers & package config.
    if test "$my_includedir" != NO; then
        my_dirs="$my_includedir $my_includedir/include"
        MY_FIND_FILE($4,$my_dirs,my_includedir,my_header)
    else
        my_dirs=`$PKG_CONFIG --variable=includedir lib$1`
        MY_FIND_FILE($4,$my_dirs,my_includedir,my_header)
    fi

    dnl find libs
    if test "$my_libdir" != NO; then
        my_dirs="$my_libdir $my_libdir/lib $my_libdir/src"
        MY_FIND_FILE(lib$1.la,$my_dirs,my_libdir)
        if test "$my_libdir" = NO; then
            AC_MSG_ERROR([
Unable to locate lib$1 library in:
    $my_dirs
Please check your installation!
            ])
        fi
        PKG_CONFIG_PATH="$my_libdir:$my_libdir/pkgconfig:$my_libdir/../unix"
        export PKG_CONFIG_PATH
    else
        my_libdir=`$PKG_CONFIG --variable=libdir lib$1`
    fi

    dnl See if pkg-config locates the library
    if $PKG_CONFIG --atleast-version=$2 lib$1; then
        :
    else
        AC_MSG_ERROR([
pkg-config couldn't locate lib$1 $2 in:
    $PKG_CONFIG_PATH
Please check your installation!
        ])
    fi

    dnl Get cflags and ldflags seperatly
    my_cxxflags=`$PKG_CONFIG --define-variable=includedir=$my_includedir --cflags lib$1`
    my_ldflags=`$PKG_CONFIG --define-variable=libdir=$my_libdir --libs lib$1`
    eval ${my_uname}_CXXFLAGS=\"$my_cxxflags\"
    eval ${my_uname}_LDFLAGS=\"$my_ldflags\"

    if test "$3" != ""; then
        my_vars="$3"
        for my_var in $my_vars; do
            my_out=${my_uname}_`echo $my_var | tr [[a-z]] [[A-Z]]`
            eval $my_out=\`$PKG_CONFIG --variable=$my_var lib$1\`
        done
    fi

    AC_MSG_RESULT([$my_libdir, $my_includedir])

    dnl Check if found library works
    MY_TRY_COMPILE($my_cxxflags,$my_ldflags,$my_header,$5,my_works)
    if test "$my_works" = NO; then
        AC_MSG_ERROR([
$1 build test failed with found library and header files.
Please check your installation!
        ])
    fi

    dnl Optional parameter - return used header
    if test "$6" != ""; then
        $6=$my_header
    fi

    dnl Restore path for next library
    PKG_CONFIG_PATH=$my_save_pkg_config_path
    export PKG_CONFIG_PATH
])


dnl -------------------------------------------------------------------------
dnl Find library on the system.  You will automatically LDFLAGS & CXXFLAGS.
dnl Results in uppercase output variables $library_$variable.
dnl $1 - library
dnl $2 - additional include paths
dnl $3 - additional library paths
dnl $4 - library header
dnl $5 - program body
dnl [$6 - used header]
dnl -------------------------------------------------------------------------
AC_DEFUN([MY_FIND_LIB],
[
    AC_MSG_CHECKING([for working $1 library and headers])
    
    if test "$my_def_prefix" = ""; then
        MY_EXPAND_PREFIX
    fi

    dnl Be pessimistic.
    my_libdir=""
    my_includedir=""
    my_uname=LIB`echo $1 | tr [[a-z]] [[A-Z]]`

    AC_ARG_WITH($1,
        [  --with-$1=DIR
            where the $1 is located],
        [my_includedir="$withval"
         my_libdir="$withval"
        ]
    )

    AC_ARG_WITH($1-includes,
        [  --with-$1-includes=DIR
            where the $1 includes are located],
        [my_includedir="$withval"]
    )

    AC_ARG_WITH($1-library,
        [  --with-$1-library=DIR
            where the $1 library is installed],
        [my_libdir="$withval"]
    )

    # Test compilation with library and headers in standard path.
    my_ldflags="-l$1"
    my_cxxflags=""
    my_works=NO

    # Use library path given by user (if any).
    if test "$my_libdir" != ""; then
        my_dirs="$my_libdir $my_libdir/lib $my_libdir/src"
        MY_FIND_FILE(lib$1.la,$my_dirs,my_libdir)
        my_ldflags="$my_libdir/lib$1.la"
    fi

    if test "$my_libdir" != NO; then
        # Use include path given by user (if any).
        if test "$my_includedir" != ""; then
            my_dirs="$my_includedir $my_includedir/include"
            MY_FIND_FILE($4,$my_dirs,my_includedir,my_header)
            if test "$my_includedir" != NO; then
                my_cxxflags="-I$my_includedir"
                # Run test compilation.
                MY_TRY_COMPILE($my_cxxflags,$my_ldflags,$my_header,$5,my_works)
            fi
        else
            # Header files are on the system so run compile tests until
            # we find the correct one
            for i in $4; do
                MY_TRY_COMPILE($my_cxxflags,$my_ldflags,$i,$5,my_works)
                if test "$my_works" != NO; then
                    my_header="$i"
                    break
                fi
            done
        fi
    fi

    if test "$my_works" = NO; then
        # Test compilation failed.
        # Need to search for library and headers
        # Search common locations where header files might be stored.
        my_dirs="$2 $my_def_includedir $my_def_prefix/include \
                 /usr/include /usr/local/include /usr/lib/$1/include \
                 /usr/local/lib/$1/include"
        MY_FIND_FILE($4,$my_dirs,my_includedir,my_header)
        my_cxxflags="-I$my_includedir"

        # Search common locations where library might be stored.
        my_dirs="$3 $my_def_libdir $my_def_prefix/lib \
                 /usr/lib /usr/local/lib /usr/lib/$1/lib /usr/local/lib/$1/lib"
        MY_FIND_FILE(lib$1.la,$my_dirs,my_libdir)
        my_ldflags="$my_libdir/lib$1.la"

        if test "$my_includedir" != NO && test "$my_libdir" != NO; then
            # Test compilation with found paths.
            MY_TRY_COMPILE($my_cxxflags,$my_ldflags,$my_header,$5,my_works)
        fi

        AC_MSG_RESULT([library $my_libdir, headers $my_includedir])
        if test "$my_works" = NO; then
            AC_MSG_ERROR([
$1 build test failed with found library and header files.
Please check your installation!
            ])
        fi
    else
        AC_MSG_RESULT(yes)
    fi

    eval ${my_uname}_CXXFLAGS=\"$my_cxxflags\"
    eval ${my_uname}_LDFLAGS=\"$my_ldflags\"

    dnl Optional parameter - return used header
    if test "$6" != ""; then
        $6=$my_header
    fi
])


dnl -------------------------------------------------------------------------
dnl Find library on the system.  This is used to find libraries that are not
dnl built/installed.  The include, lib path must be provided individually and it
dnl is not possible to validate the library path.
dnl $1 - library
dnl $2 - library header
dnl [$3 - used header]
dnl -------------------------------------------------------------------------
AC_DEFUN([MY_FIND_LIB_NO_CHECK],
[
    AC_MSG_CHECKING([for $1 library and headers])

    my_libdir=""
    my_includedir=""
    my_uname=LIB`echo $1 | tr [[a-z]] [[A-Z]]`

    AC_ARG_WITH($1-includes,
        [  --with-$1-includes=DIR
            where the $1 includes are located],
        [my_includedir="$withval"]
    )

    AC_ARG_WITH($1-library,
        [  --with-$1-library=DIR
            where the $1 library is installed],
        [my_libdir="$withval"]
    )

    dnl Both paths must be provided
    if test "$my_libdir" = ""; then
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([
$1 library path not supplied!
        ])
    fi

    if test "$my_includedir" = ""; then
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([
$1 include path not supplied!
        ])
    fi

    dnl Check the include path
    my_dirs="$my_includedir"
    MY_FIND_FILE($2,$my_dirs,my_includedir,my_header)

    if test "$my_includedir" = NO; then
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([
$1 headers not found!
        ])
    fi

    AC_MSG_RESULT(yes)
    eval ${my_uname}_CXXFLAGS=\"-I$my_includedir\"
    eval ${my_uname}_LDFLAGS=\"$my_libdir/lib$1.la\"

    dnl Optional parameter - return used header
    if test "$3" != ""; then
        $3=$my_header
    fi
])
