dnl TiMidity++ -- MIDI to WAVE converter and player
dnl Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
dnl Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>
dnl 
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl 
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

dnl MY_DEFINE(VARIABLE)
AC_DEFUN(MY_DEFINE,
[cat >> confdefs.h <<EOF
[#define] $1 1
EOF
])

dnl CONFIG_INTERFACE(package,macro_name,interface_id,help
dnl                  $1      $2         $3           $4
dnl                  action-if-yes-or-dynamic,
dnl		     $5
dnl		     action-if-yes,action-if-dynamic,action-if-no)
dnl		     $6            $7                $8
AC_DEFUN(CONFIG_INTERFACE,
[AC_ARG_ENABLE($1,[$4],
[case "x$enable_$1" in xyes|xdynamic) $5 ;; esac])
case "x$enable_$1" in
xyes)
  MY_DEFINE(IA_$2)
  AM_CONDITIONAL(ENABLE_$2, true)
  $6
  ;;
xdynamic)
  dynamic_targets="$dynamic_targets interface_$3.\$(so)"
  $7
  ;;
*)
  $8
  ;;
esac
AC_SUBST($3_so_libs)
])

dnl CHECK_DLSYM_UNDERSCORE([ACTION-IF-NEED [, ACTION IF-NOT-NEED]])
dnl variable input:
dnl   CC CFLAGS CPPFLAGS LDFLAGS LIBS SHCFLAGS SHLD SHLDFLAGS
dnl   ac_cv_header_dlfcn_h lib_dl_opt so
AC_DEFUN(CHECK_DLSYM_UNDERSCORE,
[dnl Check if dlsym need a leading underscore
AC_MSG_CHECKING(whether your dlsym() needs a leading underscore)
AC_CACHE_VAL(timidity_cv_func_dlsym_underscore,
[case "$ac_cv_header_dlfcn_h" in
yes) i_dlfcn=define;;
*)   i_dlfcn=undef;;
esac
cat > dyna.c <<EOM
fred () { }
EOM

cat > fred.c <<EOM
#include <stdio.h>
#$i_dlfcn I_DLFCN
#ifdef I_DLFCN
#include <dlfcn.h>      /* the dynamic linker include file for Sunos/Solaris */
#else
#include <sys/types.h>
#include <nlist.h>
#include <link.h>
#endif

extern int fred() ;

main()
{
    void * handle ;
    void * symbol ;
#ifndef RTLD_LAZY
    int mode = 1 ;
#else
    int mode = RTLD_LAZY ;
#endif
    handle = dlopen("./dyna.$so", mode) ;
    if (handle == NULL) {
	printf ("1\n") ;
	fflush (stdout) ;
	exit(0);
    }
    symbol = dlsym(handle, "fred") ;
    if (symbol == NULL) {
	/* try putting a leading underscore */
	symbol = dlsym(handle, "_fred") ;
	if (symbol == NULL) {
	    printf ("2\n") ;
	    fflush (stdout) ;
	    exit(0);
	}
	printf ("3\n") ;
    }
    else
	printf ("4\n") ;
    fflush (stdout) ;
    exit(0);
}
EOM
: Call the object file tmp-dyna.o in case dlext=o.
if ${CC-cc} $CFLAGS $SHCFLAGS $CPPFLAGS -c dyna.c > /dev/null 2>&1 &&
	mv dyna.o tmp-dyna.o > /dev/null 2>&1 &&
	$SHLD $SHLDFLAGS -o dyna.$so tmp-dyna.o > /dev/null 2>&1 &&
	${CC-cc} -o fred $CFLAGS $CPPFLAGS $LDFLAGS fred.c $LIBS $lib_dl_opt > /dev/null 2>&1; then
	xxx=`./fred`
	case $xxx in
	1)	AC_MSG_WARN(Test program failed using dlopen.  Perhaps you should not use dynamic loading.)
		;;
	2)	AC_MSG_WARN(Test program failed using dlsym.  Perhaps you should not use dynamic loading.)
		;;
	3)	timidity_cv_func_dlsym_underscore=yes
		;;
	4)	timidity_cv_func_dlsym_underscore=no
		;;
	esac
else
	AC_MSG_WARN(I can't compile and run the test program.)
fi
rm -f dyna.c dyna.o dyna.$so tmp-dyna.o fred.c fred.o fred
])
case "x$timidity_cv_func_dlsym_underscore" in
xyes)	[$1]
	AC_MSG_RESULT(yes)
	;;
xno)	[$2]
	AC_MSG_RESULT(no)
	;;
esac
])


dnl contains program from perl5
dnl CONTAINS_INIT()
AC_DEFUN(CONTAINS_INIT,
[dnl Some greps do not return status, grrr.
AC_MSG_CHECKING(whether grep returns status)
echo "grimblepritz" >grimble
if grep blurfldyick grimble >/dev/null 2>&1 ; then
	contains="./contains"
elif grep grimblepritz grimble >/dev/null 2>&1 ; then
	contains=grep
else
	contains="./contains"
fi
rm -f grimble
dnl the following should work in any shell
case "$contains" in
grep)	AC_MSG_RESULT(yes)
	;;
./contains)
	AC_MSG_RESULT(AGH!  Grep doesn't return a status.  Attempting remedial action.)
	cat >./contains <<'EOSS'
grep "[$]1" "[$]2" >.greptmp && cat .greptmp && test -s .greptmp
EOSS
	chmod +x "./contains"
	;;
esac
])

dnl CONTAINS(word,filename,action-if-found,action-if-not-found)
AC_DEFUN(CONTAINS,
[if $contains "^[$1]"'[$]' $2 >/dev/null 2>&1; then
  [$3]
else
  [$4]
fi
])

dnl SET_UNIQ_WORDS(shell-variable,words...)
AC_DEFUN(SET_UNIQ_WORDS,
[rm -f wordtmp >/dev/null 2>&1
val=''
for f in $2; do
  CONTAINS([$f],wordtmp,:,[echo $f >>wordtmp; val="$val $f"])
done
$1="$val"
rm -f wordtmp >/dev/null 2>&1
])


dnl WAPI_CHECK_FUNC(FUNCTION, INCLUDES, TEST-BODY,
		    [ACTION-FI-FOUND [, ACTION-IF-NOT-FOUND]])
AC_DEFUN(WAPI_CHECK_FUNC,
[AC_MSG_CHECKING(for $1)
AC_CACHE_VAL(wapi_cv_func_$1,
[AC_TRY_LINK([#include <windows.h>
$2
], [$3],
wapi_cv_func_$1=yes, wapi_cv_func_$1=no)])
if eval "test \"`echo '$wapi_cv_func_'$1`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$4], , :, [$4])
else
  AC_MSG_RESULT(no)
ifelse([$5], , , [$5
])dnl
fi
])

dnl WAPI_CHECK_LIB(LIBRARY, FUNCTION,
dnl		INCLUDES, TEST-BODY
dnl		[, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND
dnl		[, OTHER-LIBRARIES]]])
AC_DEFUN(WAPI_CHECK_LIB,
[AC_MSG_CHECKING([for $2 in -l$1])
ac_lib_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
AC_CACHE_VAL(wapi_cv_lib_$ac_lib_var,
[ac_save_LIBS="$LIBS"
LIBS="-l$1 $7 $LIBS"
AC_TRY_LINK([#include <windows.h>
$3
], [$4],
eval "wapi_cv_lib_$ac_lib_var=yes",
eval "wapi_cv_lib_$ac_lib_var=no")
LIBS="$ac_save_LIBS"
])dnl
if eval "test \"`echo '$wapi_cv_lib_'$ac_lib_var`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$5], ,LIBS="-l$1 $LIBS", [$5])
else
  AC_MSG_RESULT(no)
ifelse([$6], , , [$6
])dnl
fi
])

dnl EXTRACT_CPPFLAGS(CPPFLAGS-to-append,others-to-append,FLAGS)
AC_DEFUN(EXTRACT_CPPFLAGS,
[for f in $3; do
    case ".$f" in
	.-I?*|.-D?*)	$1="[$]$1 $f" ;;
	*)		$2="[$]$1 $f" ;;
    esac
done
])


dnl CHECK_COMPILER_OPTION(OPTIONS [, ACTION-IF-SUCCEED [, ACTION-IF-FAILED]])
AC_DEFUN(CHECK_COMPILER_OPTION,
[AC_MSG_CHECKING([whether -$1 option is recognized])
ac_ccoption=`echo $1 | sed 'y%./+-%__p_%'`
AC_CACHE_VAL(timidity_cv_ccoption_$ac_ccoption,
[cat > conftest.$ac_ext <<EOF
int main() {return 0;}
EOF
if ${CC-cc} $LDFLAGS $CFLAGS -o conftest${ac_exeext} -$1 conftest.$ac_ext > conftest.out 2>&1; then
    if test -s conftest.out; then
	eval "timidity_cv_ccoption_$ac_ccoption=no"
    else
	eval "timidity_cv_ccoption_$ac_ccoption=yes"
    fi
else
    eval "timidity_cv_ccoption_$ac_ccoption=no"
fi
])
if eval "test \"`echo '$timidity_cv_ccoption_'$ac_ccoption`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$2], , , [$2
])
else
  AC_MSG_RESULT(no)
ifelse([$3], , , [$3
])
fi
])


dnl MY_SEARCH_LIBS(FUNCTION, LIBRARIES [, ACTION-IF-FOUND
dnl            [, ACTION-IF-NOT-FOUND [, OTHER-LIBRARIES]]])
dnl Search for a library defining FUNC, if it's not already available.

AC_DEFUN(MY_SEARCH_LIBS,
[AC_CACHE_CHECK([for library containing $1], [timidity_cv_search_$1],
[ac_func_search_save_LIBS="$LIBS"
timidity_cv_search_$1="no"
for i in $2; do
  LIBS="$i $5 $ac_func_search_save_LIBS"
  AC_TRY_LINK_FUNC([$1], [timidity_cv_search_$1="$i"; break])
done
LIBS="$ac_func_search_save_LIBS"])
if test "$timidity_cv_search_$1" != "no"; then
  $3
else :
  $4
fi])