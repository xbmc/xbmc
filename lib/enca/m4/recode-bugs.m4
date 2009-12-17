## GNU recode well-known bugs test.  This file is in public domain.
## Defines:
## RECODE_PROG_BUGGY when at least one of the bugs is present
##
## Checks for:
## whether conversion to UTF-7 loses the `-' in `+-'
##   (reported in Feb 2001)
## whether recode crashes on i/o errors
##   (reported in Feb 2001)
## whether conversion to UCS*/CRLF forgets the last EOL
##   (reported in Mar 2001)
## whether conversion to UTF-8 corrupts files
##   (reported in Mar 2001)
## whether recode crashes on TeX/..ISO-8859-2 conversions
##   (reported in Jul 2001)
## whether recode MacCE map is broken for Latvian
## whether recode built-in KOI8-U map is broken
## whether recode ISO8859-13 to Unicode map is broken.

## GNU recode broken test.
## Prints a warning for broken recode -- no sensible workaround known ;-(
## Maybe we should at least not to use recode for some conversions.
## Defines:
## RECODE_PROG_BUGGY when at least one of the bugs is present
AC_DEFUN([ye_RECODE_BROKEN],
[dnl Check for recode bugs and print a warning if found.
ye_RECODE_BROKEN_UTF8PM
if test "$yeti_cv_prog_recode_utf7pm" = "yes"; then yeti_recode_buggy=yes; fi
ye_RECODE_BROKEN_IOSEGV
if test "$yeti_cv_prog_recode_iosegv" = "yes"; then yeti_recode_buggy=yes; fi
ye_RECODE_BROKEN_UCSCRLF
if test "$yeti_cv_prog_recode_ucscrlf" = "yes"; then yeti_recode_buggy=yes; fi
ye_RECODE_BROKEN_UTF8CORR
if test "$yeti_cv_prog_recode_utf8corr" = "yes"; then yeti_recode_buggy=yes; fi
ye_RECODE_BROKEN_TEXL2SEGV
if test "$yeti_cv_prog_recode_texl2segv" = "yes"; then yeti_recode_buggy=yes; fi
ye_RECODE_BROKEN_MACCELV
if test "$yeti_cv_prog_recode_maccelv" = "yes"; then yeti_recode_buggy=yes; fi
ye_RECODE_BROKEN_KOI8U
if test "$yeti_cv_prog_recode_koi8u" = "yes"; then yeti_recode_buggy=yes; fi
ye_RECODE_BROKEN_ISO885913
if test "$yeti_cv_prog_recode_iso885913" = "yes"; then yeti_recode_buggy=yes; fi
if test "$yeti_recode_buggy" = "yes"; then
  AC_DEFINE(RECODE_PROG_BUGGY,1,[Define if the recode recoder is incredibily buggy.])
fi])

## GNU recode bug test
AC_DEFUN([ye_RECODE_BROKEN_UTF8PM],
[dnl Check whether conversion to UTF-7 loses the - in +-
AC_CACHE_CHECK([whether recode loses - in +- in to-UTF-7 conversion],
  yeti_cv_prog_recode_utf7pm,
  if test "`echo +- | recode l1..utf7 | recode utf7..l1`" = "+-"; then
    yeti_cv_prog_recode_utf7pm=no
  else
    yeti_cv_prog_recode_utf7pm=yes
  fi)
])

## GNU recode bug test
AC_DEFUN([ye_RECODE_BROKEN_IOSEGV],
[dnl Check whether recode crashes on i/o errors
dnl FIXME: this obviously can't work when one is root!
AC_CACHE_CHECK([whether recode crashes on i/o errors],
  yeti_cv_prog_recode_iosegv,
  rm -f core*
  mkdir ac_tmp_dir
  echo >ac_tmp_dir/ac_tmp.txt
  chmod 0555 ac_tmp_dir
  yeti_cv_prog_recode_iosegv=no
  { recode l2..ascii ac_tmp_dir/ac_tmp.txt; } 2>&5 || yeti_cv_prog_recode_iosegv=yes
  if test -n "`ls | grep '^core\(\.[0-9]\+\)\?'`"; then
    yeti_cv_prog_recode_iosegv=yes
  fi
  rm -f core*
  chmod 0777 ac_tmp_dir
  rm -f ac_tmp_dir/ac_tmp.txt
  rmdir ac_tmp_dir)
])

## GNU recode bug test
AC_DEFUN([ye_RECODE_BROKEN_UCSCRLF],
[dnl Check whether conversion to UCS*/CRLF forgets the last EOL
AC_CACHE_CHECK([whether conversion to UCS/CRLF results in odd-sized files],
  yeti_cv_prog_recode_ucscrlf,
  if test "`echo | recode l2..ucs2/crlf | wc -c | sed -e 's: ::g'`" = "5"; then
    yeti_cv_prog_recode_ucscrlf=yes
  else
    yeti_cv_prog_recode_ucscrlf=no
  fi)
])

## GNU recode bug test
AC_DEFUN([ye_RECODE_BROKEN_UTF8CORR],
[dnl Check whether conversion to UTF-8 corrupts files
AC_CACHE_CHECK([whether conversion to UTF-8 corrupts files],
  yeti_cv_prog_recode_utf8corr,
  recode l2..utf8 <$srcdir/m4/long-text.l2 | recode utf8..l2 >ac_tmp.txt
  if diff ac_tmp.txt $srcdir/m4/long-text.l2 1>&5 2>&5; then
    yeti_cv_prog_recode_utf8corr=no
  else
    yeti_cv_prog_recode_utf8corr=yes
  fi
  rm -f ac_tmp.txt)
])

## GNU recode bug test
AC_DEFUN([ye_RECODE_BROKEN_TEXL2SEGV],
[dnl Check whether recode crashes on TeX/..ISO-8859-2 conversion
AC_CACHE_CHECK([whether recode crashes on TeX/..ISO-8859-2 conversions],
  yeti_cv_prog_recode_texl2segv,
  rm -f core* crash-me-a crash-me-b
  cat $srcdir/m4/crash-me >crash-me-a
  cat $srcdir/m4/crash-me >crash-me-b
  yeti_cv_prog_recode_texl2segv=no
  { recode TeX/..ISO-8859-2 crash-me-a crash-me-b; } 2>&5 || yeti_cv_prog_recode_texl2segv=yes
  if test -n "`ls | grep '^core\(\.[0-9]\+\)\?'`"; then
    yeti_cv_prog_recode_texl2segv=yes
  fi
  rm -f core* crash-me-a crash-me-b)
])

## GNU recode bug test
AC_DEFUN([ye_RECODE_BROKEN_MACCELV],
[dnl Check whether recode MacCE map is broken for Latvian
AC_CACHE_CHECK([whether recode MacCE map is broken for Latvian],
  yeti_cv_prog_recode_maccelv,
  if test "`echo ì | recode 1257/..macce/`" != "®"; then
    yeti_cv_prog_recode_maccelv=yes
  else
    yeti_cv_prog_recode_maccelv=no
  fi)
])

## GNU recode bug test
AC_DEFUN([ye_RECODE_BROKEN_KOI8U],
[dnl Check whether recode KOI8-U map is broken.
AC_CACHE_CHECK([whether recode built-in KOI8-U map is broken],
  yeti_cv_prog_recode_koi8u,
  if test "`echo þ | recode -x:libiconv: koi8u..iso88595`" != "Ç"; then
    yeti_cv_prog_recode_koi8u=yes
  else
    yeti_cv_prog_recode_koi8u=no
  fi)
])

## GNU recode bug test
AC_DEFUN([ye_RECODE_BROKEN_ISO885913],
[dnl Check whether recode ISO8859-13 to Unicode map is broken.
AC_CACHE_CHECK([whether recode ISO8859-13 to Unicode map is broken],
  yeti_cv_prog_recode_iso885913,
  if test "`echo ¿ | recode iso8859-13..ucs2 | recode ucs2..l1`" = "¿"; then
    yeti_cv_prog_recode_iso885913=yes
  else
    yeti_cv_prog_recode_iso885913=no
  fi)
])
