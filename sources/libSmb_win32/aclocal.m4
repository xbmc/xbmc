dnl AC_VALIDATE_CACHE_SYSTEM_TYPE[(cmd)]
dnl if the cache file is inconsistent with the current host,
dnl target and build system types, execute CMD or print a default
dnl error message.
AC_DEFUN(AC_VALIDATE_CACHE_SYSTEM_TYPE, [
    AC_REQUIRE([AC_CANONICAL_SYSTEM])
    AC_MSG_CHECKING([config.cache system type])
    if { test x"${ac_cv_host_system_type+set}" = x"set" &&
         test x"$ac_cv_host_system_type" != x"$host"; } ||
       { test x"${ac_cv_build_system_type+set}" = x"set" &&
         test x"$ac_cv_build_system_type" != x"$build"; } ||
       { test x"${ac_cv_target_system_type+set}" = x"set" &&
         test x"$ac_cv_target_system_type" != x"$target"; }; then
	AC_MSG_RESULT([different])
	ifelse($#, 1, [$1],
		[AC_MSG_ERROR(["you must remove config.cache and restart configure"])])
    else
	AC_MSG_RESULT([same])
    fi
    ac_cv_host_system_type="$host"
    ac_cv_build_system_type="$build"
    ac_cv_target_system_type="$target"
])


dnl test whether dirent has a d_off member
AC_DEFUN(AC_DIRENT_D_OFF,
[AC_CACHE_CHECK([for d_off in dirent], ac_cv_dirent_d_off,
[AC_TRY_COMPILE([
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>], [struct dirent d; d.d_off;],
ac_cv_dirent_d_off=yes, ac_cv_dirent_d_off=no)])
if test $ac_cv_dirent_d_off = yes; then
  AC_DEFINE(HAVE_DIRENT_D_OFF,1,[Whether dirent has a d_off member])
fi
])

dnl Mark specified module as shared
dnl SMB_MODULE(name,static_files,shared_files,subsystem,whatif-static,whatif-shared)
AC_DEFUN(SMB_MODULE,
[
	AC_MSG_CHECKING([how to build $1])
	if test "$[MODULE_][$1]"; then
		DEST=$[MODULE_][$1]
	elif test "$[MODULE_]translit([$4], [A-Z], [a-z])" -a "$[MODULE_DEFAULT_][$1]"; then
		DEST=$[MODULE_]translit([$4], [A-Z], [a-z])
	else
		DEST=$[MODULE_DEFAULT_][$1]
	fi
	
	if test x"$DEST" = xSHARED; then
		AC_DEFINE([$1][_init], [init_module], [Whether to build $1 as shared module])
		$4_MODULES="$$4_MODULES $3"
		AC_MSG_RESULT([shared])
		[$6]
		string_shared_modules="$string_shared_modules $1"
	elif test x"$DEST" = xSTATIC; then
		[init_static_modules_]translit([$4], [A-Z], [a-z])="$[init_static_modules_]translit([$4], [A-Z], [a-z])  $1_init();"
 		[decl_static_modules_]translit([$4], [A-Z], [a-z])="$[decl_static_modules_]translit([$4], [A-Z], [a-z]) extern NTSTATUS $1_init(void);"
		string_static_modules="$string_static_modules $1"
		$4_STATIC="$$4_STATIC $2"
		AC_SUBST($4_STATIC)
		[$5]
		AC_MSG_RESULT([static])
	else
	    string_ignored_modules="$string_ignored_modules $1"
		AC_MSG_RESULT([not])
	fi
])

AC_DEFUN(SMB_SUBSYSTEM,
[
	AC_SUBST($1_STATIC)
	AC_SUBST($1_MODULES)
	AC_DEFINE_UNQUOTED([static_init_]translit([$1], [A-Z], [a-z]), [{$init_static_modules_]translit([$1], [A-Z], [a-z])[}], [Static init functions])
	AC_DEFINE_UNQUOTED([static_decl_]translit([$1], [A-Z], [a-z]), [$decl_static_modules_]translit([$1], [A-Z], [a-z]), [Decl of Static init functions])
    	ifelse([$2], , :, [rm -f $2])
])

dnl AC_PROG_CC_FLAG(flag)
AC_DEFUN(AC_PROG_CC_FLAG,
[AC_CACHE_CHECK(whether ${CC-cc} accepts -$1, ac_cv_prog_cc_$1,
[echo 'void f(){}' > conftest.c
if test -z "`${CC-cc} -$1 -c conftest.c 2>&1`"; then
  ac_cv_prog_cc_$1=yes
else
  ac_cv_prog_cc_$1=no
fi
rm -f conftest*
])])

dnl see if a declaration exists for a function or variable
dnl defines HAVE_function_DECL if it exists
dnl AC_HAVE_DECL(var, includes)
AC_DEFUN(AC_HAVE_DECL,
[
 AC_CACHE_CHECK([for $1 declaration],ac_cv_have_$1_decl,[
    AC_TRY_COMPILE([$2],[int i = (int)$1],
        ac_cv_have_$1_decl=yes,ac_cv_have_$1_decl=no)])
 if test x"$ac_cv_have_$1_decl" = x"yes"; then
    AC_DEFINE([HAVE_]translit([$1], [a-z], [A-Z])[_DECL],1,[Whether $1() is available])
 fi
])


dnl AC_LIBTESTFUNC(lib, function, [actions if found], [actions if not found])
dnl Check for a function in a library, but don't keep adding the same library
dnl to the LIBS variable.  Check whether the function is available in the
dnl current LIBS before adding the library which prevents us spuriously
dnl adding libraries for symbols that are in libc.
dnl
dnl On success, the default actions ensure that HAVE_FOO is defined. The lib
dnl is always added to $LIBS if it was found to be necessary. The caller
dnl can use SMB_LIB_REMOVE to strp this if necessary.
AC_DEFUN([AC_LIBTESTFUNC],
[
  AC_CHECK_FUNCS($2,
      [
        # $2 was found in libc or existing $LIBS
	ifelse($3, [],
	    [
		AC_DEFINE(translit([HAVE_$2], [a-z], [A-Z]), 1,
		    [Whether $2 is available])
	    ],
	    [
		$3
	    ])
      ],
      [
        # $2 was not found, try adding lib$1
	case " $LIBS " in
          *\ -l$1\ *)
	    ifelse($4, [],
		[
		    # $2 was not found and we already had lib$1
		    # nothing to do here by default
		    true
		],
		[ $4 ])
	    ;;
          *)
	    # $2 was not found, try adding lib$1
	    AC_CHECK_LIB($1, $2,
	      [
		LIBS="-l$1 $LIBS"
		ifelse($3, [],
		    [
			AC_DEFINE(translit([HAVE_$2], [a-z], [A-Z]), 1,
			    [Whether $2 is available])
		    ],
		    [
			$3
		    ])
	      ],
	      [
		ifelse($4, [],
		    [
			# $2 was not found in lib$1
			# nothing to do here by default
			true
		    ],
		    [ $4 ])
	      ])
	  ;;
        esac
      ])
])

# AC_CHECK_LIB_EXT(LIBRARY, [EXT_LIBS], [FUNCTION],
#              [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#              [ADD-ACTION-IF-FOUND],[OTHER-LIBRARIES])
# ------------------------------------------------------
#
# Use a cache variable name containing both the library and function name,
# because the test really is for library $1 defining function $3, not
# just for library $1.  Separate tests with the same $1 and different $3s
# may have different results.
#
# Note that using directly AS_VAR_PUSHDEF([ac_Lib], [ac_cv_lib_$1_$3])
# is asking for trouble, since AC_CHECK_LIB($lib, fun) would give
# ac_cv_lib_$lib_fun, which is definitely not what was meant.  Hence
# the AS_LITERAL_IF indirection.
#
# FIXME: This macro is extremely suspicious.  It DEFINEs unconditionally,
# whatever the FUNCTION, in addition to not being a *S macro.  Note
# that the cache does depend upon the function we are looking for.
#
# It is on purpose we used `ac_check_lib_ext_save_LIBS' and not just
# `ac_save_LIBS': there are many macros which don't want to see `LIBS'
# changed but still want to use AC_CHECK_LIB_EXT, so they save `LIBS'.
# And ``ac_save_LIBS' is too tempting a name, so let's leave them some
# freedom.
AC_DEFUN([AC_CHECK_LIB_EXT],
[
AH_CHECK_LIB_EXT([$1])
ac_check_lib_ext_save_LIBS=$LIBS
LIBS="-l$1 $$2 $7 $LIBS"
AS_LITERAL_IF([$1],
      [AS_VAR_PUSHDEF([ac_Lib_ext], [ac_cv_lib_ext_$1])],
      [AS_VAR_PUSHDEF([ac_Lib_ext], [ac_cv_lib_ext_$1''])])dnl

m4_ifval([$3],
 [
    AH_CHECK_FUNC_EXT([$3])
    AS_LITERAL_IF([$1],
              [AS_VAR_PUSHDEF([ac_Lib_func], [ac_cv_lib_ext_$1_$3])],
              [AS_VAR_PUSHDEF([ac_Lib_func], [ac_cv_lib_ext_$1''_$3])])dnl
    AC_CACHE_CHECK([for $3 in -l$1], ac_Lib_func,
	[AC_TRY_LINK_FUNC($3,
                 [AS_VAR_SET(ac_Lib_func, yes);
		  AS_VAR_SET(ac_Lib_ext, yes)],
                 [AS_VAR_SET(ac_Lib_func, no);
		  AS_VAR_SET(ac_Lib_ext, no)])
	])
    AS_IF([test AS_VAR_GET(ac_Lib_func) = yes],
        [AC_DEFINE_UNQUOTED(AS_TR_CPP(HAVE_$3))])dnl
    AS_VAR_POPDEF([ac_Lib_func])dnl
 ],[
    AC_CACHE_CHECK([for -l$1], ac_Lib_ext,
	[AC_TRY_LINK_FUNC([main],
                 [AS_VAR_SET(ac_Lib_ext, yes)],
                 [AS_VAR_SET(ac_Lib_ext, no)])
	])
 ])
LIBS=$ac_check_lib_ext_save_LIBS

AS_IF([test AS_VAR_GET(ac_Lib_ext) = yes],
    [m4_default([$4], 
        [AC_DEFINE_UNQUOTED(AS_TR_CPP(HAVE_LIB$1))
		case "$$2" in
		    *-l$1*)
			;;
		    *)
			$2="-l$1 $$2"
			;;
		esac])
		[$6]
	    ],
	    [$5])dnl
AS_VAR_POPDEF([ac_Lib_ext])dnl
])# AC_CHECK_LIB_EXT

# AH_CHECK_LIB_EXT(LIBNAME)
# ---------------------
m4_define([AH_CHECK_LIB_EXT],
[AH_TEMPLATE(AS_TR_CPP(HAVE_LIB$1),
             [Define to 1 if you have the `]$1[' library (-l]$1[).])])

# AC_CHECK_FUNCS_EXT(FUNCTION, [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# -----------------------------------------------------------------
dnl check for a function in a $LIBS and $OTHER_LIBS libraries variable.
dnl AC_CHECK_FUNC_EXT(func,OTHER_LIBS,IF-TRUE,IF-FALSE)
AC_DEFUN([AC_CHECK_FUNC_EXT],
[
    AH_CHECK_FUNC_EXT($1)	
    ac_check_func_ext_save_LIBS=$LIBS
    LIBS="$2 $LIBS"
    AS_VAR_PUSHDEF([ac_var], [ac_cv_func_ext_$1])dnl
    AC_CACHE_CHECK([for $1], ac_var,
	[AC_LINK_IFELSE([AC_LANG_FUNC_LINK_TRY([$1])],
                [AS_VAR_SET(ac_var, yes)],
                [AS_VAR_SET(ac_var, no)])])
    LIBS=$ac_check_func_ext_save_LIBS
    AS_IF([test AS_VAR_GET(ac_var) = yes], 
	    [AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_$1])) $3], 
	    [$4])dnl
AS_VAR_POPDEF([ac_var])dnl
])# AC_CHECK_FUNC

# AH_CHECK_FUNC_EXT(FUNCNAME)
# ---------------------
m4_define([AH_CHECK_FUNC_EXT],
[AH_TEMPLATE(AS_TR_CPP(HAVE_$1),
             [Define to 1 if you have the `]$1[' function.])])

dnl Define an AC_DEFINE with ifndef guard.
dnl AC_N_DEFINE(VARIABLE [, VALUE])
define(AC_N_DEFINE,
[cat >> confdefs.h <<\EOF
[#ifndef] $1
[#define] $1 ifelse($#, 2, [$2], $#, 3, [$2], 1)
[#endif]
EOF
])

dnl Add an #include
dnl AC_ADD_INCLUDE(VARIABLE)
define(AC_ADD_INCLUDE,
[cat >> confdefs.h <<\EOF
[#include] $1
EOF
])

dnl Copied from libtool.m4
AC_DEFUN(AC_PROG_LD_GNU,
[AC_CACHE_CHECK([if the linker ($LD) is GNU ld], ac_cv_prog_gnu_ld,
[# I'd rather use --version here, but apparently some GNU ld's only accept -v.
if $LD -v 2>&1 </dev/null | egrep '(GNU|with BFD)' 1>&5; then
  ac_cv_prog_gnu_ld=yes
else
  ac_cv_prog_gnu_ld=no
fi])
])

# Configure paths for LIBXML2
# Toshio Kuratomi 2001-04-21
# Adapted from:
# Configure paths for GLIB
# Owen Taylor     97-11-3

dnl AM_PATH_XML2([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for XML, and define XML_CFLAGS and XML_LIBS
dnl
AC_DEFUN(AM_PATH_XML2,[ 
AC_ARG_WITH(xml-prefix,
            [  --with-xml-prefix=PFX   Prefix where libxml is installed (optional)],
            xml_config_prefix="$withval", xml_config_prefix="")
AC_ARG_WITH(xml-exec-prefix,
            [  --with-xml-exec-prefix=PFX Exec prefix where libxml is installed (optional)],
            xml_config_exec_prefix="$withval", xml_config_exec_prefix="")
AC_ARG_ENABLE(xmltest,
              [  --disable-xmltest       Do not try to compile and run a test LIBXML program],,
              enable_xmltest=yes)

  if test x$xml_config_exec_prefix != x ; then
     xml_config_args="$xml_config_args --exec-prefix=$xml_config_exec_prefix"
     if test x${XML2_CONFIG+set} != xset ; then
        XML2_CONFIG=$xml_config_exec_prefix/bin/xml2-config
     fi
  fi
  if test x$xml_config_prefix != x ; then
     xml_config_args="$xml_config_args --prefix=$xml_config_prefix"
     if test x${XML2_CONFIG+set} != xset ; then
        XML2_CONFIG=$xml_config_prefix/bin/xml2-config
     fi
  fi

  AC_PATH_PROG(XML2_CONFIG, xml2-config, no)
  min_xml_version=ifelse([$1], ,2.0.0,[$1])
  AC_MSG_CHECKING(for libxml - version >= $min_xml_version)
  no_xml=""
  if test "$XML2_CONFIG" = "no" ; then
    no_xml=yes
  else
    XML_CFLAGS=`$XML2_CONFIG $xml_config_args --cflags`
    XML_LIBS=`$XML2_CONFIG $xml_config_args --libs`
    xml_config_major_version=`$XML2_CONFIG $xml_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    xml_config_minor_version=`$XML2_CONFIG $xml_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    xml_config_micro_version=`$XML2_CONFIG $xml_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_xmltest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $XML_CFLAGS"
      LIBS="$XML_LIBS $LIBS"
dnl
dnl Now check if the installed libxml is sufficiently new.
dnl (Also sanity checks the results of xml2-config to some extent)
dnl
      rm -f conf.xmltest
      AC_TRY_RUN([
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libxml/xmlversion.h>

int 
main()
{
  int xml_major_version, xml_minor_version, xml_micro_version;
  int major, minor, micro;
  char *tmp_version;

  system("touch conf.xmltest");

  /* Capture xml2-config output via autoconf/configure variables */
  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = (char *)strdup("$min_xml_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string from xml2-config\n", "$min_xml_version");
     exit(1);
   }
   free(tmp_version);

   /* Capture the version information from the header files */
   tmp_version = (char *)strdup(LIBXML_DOTTED_VERSION);
   if (sscanf(tmp_version, "%d.%d.%d", &xml_major_version, &xml_minor_version, &xml_micro_version) != 3) {
     printf("%s, bad version string from libxml includes\n", "LIBXML_DOTTED_VERSION");
     exit(1);
   }
   free(tmp_version);

 /* Compare xml2-config output to the libxml headers */
  if ((xml_major_version != $xml_config_major_version) ||
      (xml_minor_version != $xml_config_minor_version) ||
      (xml_micro_version != $xml_config_micro_version))
    {
      printf("*** libxml header files (version %d.%d.%d) do not match\n",
         xml_major_version, xml_minor_version, xml_micro_version);
      printf("*** xml2-config (version %d.%d.%d)\n",
         $xml_config_major_version, $xml_config_minor_version, $xml_config_micro_version);
      return 1;
    } 
/* Compare the headers to the library to make sure we match */
  /* Less than ideal -- doesn't provide us with return value feedback, 
   * only exits if there's a serious mismatch between header and library.
   */
    LIBXML_TEST_VERSION;

    /* Test that the library is greater than our minimum version */
    if ((xml_major_version > major) ||
        ((xml_major_version == major) && (xml_minor_version > minor)) ||
        ((xml_major_version == major) && (xml_minor_version == minor) &&
        (xml_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of libxml (%d.%d.%d) was found.\n",
               xml_major_version, xml_minor_version, xml_micro_version);
        printf("*** You need a version of libxml newer than %d.%d.%d. The latest version of\n",
           major, minor, micro);
        printf("*** libxml is always available from ftp://ftp.xmlsoft.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the xml2-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of LIBXML, but you can also set the XML2_CONFIG environment to point to the\n");
        printf("*** correct copy of xml2-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
    }
  return 1;
}
],, no_xml=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi

  if test "x$no_xml" = x ; then
     AC_MSG_RESULT(yes (version $xml_config_major_version.$xml_config_minor_version.$xml_config_micro_version))
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$XML2_CONFIG" = "no" ; then
       echo "*** The xml2-config script installed by LIBXML could not be found"
       echo "*** If libxml was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the XML2_CONFIG environment variable to the"
       echo "*** full path to xml2-config."
     else
       if test -f conf.xmltest ; then
        :
       else
          echo "*** Could not run libxml test program, checking why..."
          CFLAGS="$CFLAGS $XML_CFLAGS"
          LIBS="$LIBS $XML_LIBS"
          AC_TRY_LINK([
#include <libxml/xmlversion.h>
#include <stdio.h>
],      [ LIBXML_TEST_VERSION; return 0;],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding LIBXML or finding the wrong"
          echo "*** version of LIBXML. If it is not finding LIBXML, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
          echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means LIBXML was incorrectly installed"
          echo "*** or that you have moved LIBXML since it was installed. In the latter case, you"
          echo "*** may want to edit the xml2-config script: $XML2_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi

     XML_CFLAGS=""
     XML_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(XML_CFLAGS)
  AC_SUBST(XML_LIBS)
  rm -f conf.xmltest
])

# =========================================================================
# AM_PATH_MYSQL : MySQL library

dnl AM_PATH_MYSQL([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for MYSQL, and define MYSQL_CFLAGS and MYSQL_LIBS
dnl
AC_DEFUN(AM_PATH_MYSQL,
[dnl
dnl Get the cflags and libraries from the mysql_config script
dnl
AC_ARG_WITH(mysql-prefix,[  --with-mysql-prefix=PFX   Prefix where MYSQL is installed (optional)],
            mysql_prefix="$withval", mysql_prefix="")
AC_ARG_WITH(mysql-exec-prefix,[  --with-mysql-exec-prefix=PFX Exec prefix where MYSQL is installed (optional)],
            mysql_exec_prefix="$withval", mysql_exec_prefix="")

  if test x$mysql_exec_prefix != x ; then
     mysql_args="$mysql_args --exec-prefix=$mysql_exec_prefix"
     if test x${MYSQL_CONFIG+set} != xset ; then
        MYSQL_CONFIG=$mysql_exec_prefix/bin/mysql_config
     fi
  fi
  if test x$mysql_prefix != x ; then
     mysql_args="$mysql_args --prefix=$mysql_prefix"
     if test x${MYSQL_CONFIG+set} != xset ; then
        MYSQL_CONFIG=$mysql_prefix/bin/mysql_config
     fi
  fi

  AC_REQUIRE([AC_CANONICAL_TARGET])
  AC_PATH_PROG(MYSQL_CONFIG, mysql_config, no)
  AC_MSG_CHECKING(for MYSQL)
  no_mysql=""
  if test "$MYSQL_CONFIG" = "no" ; then
    MYSQL_CFLAGS=""
    MYSQL_LIBS=""
    AC_MSG_RESULT(no)
     ifelse([$2], , :, [$2])
  else
    MYSQL_CFLAGS=`$MYSQL_CONFIG $mysqlconf_args --cflags | sed -e "s/'//g"`
    MYSQL_LIBS=`$MYSQL_CONFIG $mysqlconf_args --libs | sed -e "s/'//g"`
    AC_MSG_RESULT(yes)
    ifelse([$1], , :, [$1])
  fi
  AC_SUBST(MYSQL_CFLAGS)
  AC_SUBST(MYSQL_LIBS)
])

# =========================================================================
# AM_PATH_PGSQL : pgSQL library

dnl AM_PATH_PGSQL([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for PGSQL, and define PGSQL_CFLAGS and PGSQL_LIBS
dnl
AC_DEFUN(AM_PATH_PGSQL,
[dnl
dnl Get the cflags and libraries from the pg_config script
dnl
AC_ARG_WITH(pgsql-prefix,[  --with-pgsql-prefix=PFX   Prefix where PostgreSQL is installed (optional)],
            pgsql_prefix="$withval", pgsql_prefix="")
AC_ARG_WITH(pgsql-exec-prefix,[  --with-pgsql-exec-prefix=PFX Exec prefix where PostgreSQL is installed (optional)],
            pgsql_exec_prefix="$withval", pgsql_exec_prefix="")

  if test x$pgsql_exec_prefix != x ; then
     if test x${PGSQL_CONFIG+set} != xset ; then
        PGSQL_CONFIG=$pgsql_exec_prefix/bin/pg_config
     fi
  fi
  if test x$pgsql_prefix != x ; then
     if test x${PGSQL_CONFIG+set} != xset ; then
        PGSQL_CONFIG=$pgsql_prefix/bin/pg_config
     fi
  fi

  AC_REQUIRE([AC_CANONICAL_TARGET])
  AC_PATH_PROG(PGSQL_CONFIG, pg_config, no, [$PATH:/usr/lib/postgresql/bin])
  AC_MSG_CHECKING(for PGSQL)
  no_pgsql=""
  if test "$PGSQL_CONFIG" = "no" ; then
    PGSQL_CFLAGS=""
    PGSQL_LIBS=""
    AC_MSG_RESULT(no)
     ifelse([$2], , :, [$2])
  else
    PGSQL_CFLAGS=-I`$PGSQL_CONFIG --includedir`
    PGSQL_LIBS="-lpq -L`$PGSQL_CONFIG --libdir`"
    AC_MSG_RESULT(yes)
    ifelse([$1], , :, [$1])
  fi
  AC_SUBST(PGSQL_CFLAGS)
  AC_SUBST(PGSQL_LIBS)
])

dnl Removes -I/usr/include/? from given variable
AC_DEFUN(CFLAGS_REMOVE_USR_INCLUDE,[
  ac_new_flags=""
  for i in [$]$1; do
    case [$]i in
    -I/usr/include|-I/usr/include/) ;;
    *) ac_new_flags="[$]ac_new_flags [$]i" ;;
    esac
  done
  $1=[$]ac_new_flags
])
    
dnl Removes -L/usr/lib/? from given variable
AC_DEFUN(LIB_REMOVE_USR_LIB,[
  ac_new_flags=""
  for i in [$]$1; do
    case [$]i in
    -L/usr/lib|-L/usr/lib/) ;;
    *) ac_new_flags="[$]ac_new_flags [$]i" ;;
    esac
  done
  $1=[$]ac_new_flags
])

dnl From Bruno Haible.

AC_DEFUN(jm_ICONV,
[
  dnl Some systems have iconv in libc, some have it in libiconv (OSF/1 and
  dnl those with the standalone portable libiconv installed).
  AC_MSG_CHECKING(for iconv in $1)
    jm_cv_func_iconv="no"
    jm_cv_lib_iconv=""
    jm_cv_giconv=no
    jm_save_LIBS="$LIBS"

    dnl Check for include in funny place but no lib needed
    if test "$jm_cv_func_iconv" != yes; then 
      AC_TRY_LINK([#include <stdlib.h>
#include <giconv.h>],
        [iconv_t cd = iconv_open("","");
         iconv(cd,NULL,NULL,NULL,NULL);
         iconv_close(cd);],
         jm_cv_func_iconv=yes
         jm_cv_include="giconv.h"
         jm_cv_giconv="yes"
         jm_cv_lib_iconv="")

      dnl Standard iconv.h include, lib in glibc or libc ...
      if test "$jm_cv_func_iconv" != yes; then
        AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
          [iconv_t cd = iconv_open("","");
           iconv(cd,NULL,NULL,NULL,NULL);
           iconv_close(cd);],
           jm_cv_include="iconv.h"
           jm_cv_func_iconv=yes
           jm_cv_lib_iconv="")

          if test "$jm_cv_lib_iconv" != yes; then
            jm_save_LIBS="$LIBS"
            LIBS="$LIBS -lgiconv"
            AC_TRY_LINK([#include <stdlib.h>
#include <giconv.h>],
              [iconv_t cd = iconv_open("","");
               iconv(cd,NULL,NULL,NULL,NULL);
               iconv_close(cd);],
              jm_cv_lib_iconv=yes
              jm_cv_func_iconv=yes
              jm_cv_include="giconv.h"
              jm_cv_giconv=yes
              jm_cv_lib_iconv="giconv")

           LIBS="$jm_save_LIBS"

        if test "$jm_cv_func_iconv" != yes; then
          jm_save_LIBS="$LIBS"
          LIBS="$LIBS -liconv"
          AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
            [iconv_t cd = iconv_open("","");
             iconv(cd,NULL,NULL,NULL,NULL);
             iconv_close(cd);],
            jm_cv_include="iconv.h"
            jm_cv_func_iconv=yes
            jm_cv_lib_iconv="iconv")
          LIBS="$jm_save_LIBS"
        fi
      fi
    fi
  fi
  if test "$jm_cv_func_iconv" = yes; then
    if test "$jm_cv_giconv" = yes; then
      AC_DEFINE(HAVE_GICONV, 1, [What header to include for iconv() function: giconv.h])
      AC_MSG_RESULT(yes)
      ICONV_FOUND=yes
    else
      if test "$jm_cv_biconv" = yes; then
        AC_DEFINE(HAVE_BICONV, 1, [What header to include for iconv() function: biconv.h])
        AC_MSG_RESULT(yes)
        ICONV_FOUND=yes
      else 
        AC_DEFINE(HAVE_ICONV, 1, [What header to include for iconv() function: iconv.h])
        AC_MSG_RESULT(yes)
        ICONV_FOUND=yes
      fi
    fi
  else
    AC_MSG_RESULT(no)
  fi
])

AC_DEFUN(rjs_CHARSET,[
  dnl Find out if we can convert from $1 to UCS2-LE
  AC_MSG_CHECKING([can we convert from $1 to UCS2-LE?])
  AC_TRY_RUN([
#include <$jm_cv_include>
main(){
    iconv_t cd = iconv_open("$1", "UCS-2LE");
    if (cd == 0 || cd == (iconv_t)-1) {
	return -1;
    }
    return 0;
}
  ],ICONV_CHARSET=$1,ICONV_CHARSET=no,ICONV_CHARSET=cross)
  AC_MSG_RESULT($ICONV_CHARSET)
])

dnl CFLAGS_ADD_DIR(CFLAGS, $INCDIR)
dnl This function doesn't add -I/usr/include into CFLAGS
AC_DEFUN(CFLAGS_ADD_DIR,[
if test "$2" != "/usr/include" ; then
    $1="$$1 -I$2"
fi
])

dnl LIB_ADD_DIR(LDFLAGS, $LIBDIR)
dnl This function doesn't add -L/usr/lib into LDFLAGS
AC_DEFUN(LIB_ADD_DIR,[
if test "$2" != "/usr/lib" ; then
    $1="$$1 -L$2"
fi
])

dnl AC_ENABLE_SHARED - implement the --enable-shared flag
dnl Usage: AC_ENABLE_SHARED[(DEFAULT)]
dnl   Where DEFAULT is either `yes' or `no'.  If omitted, it defaults to
dnl   `yes'.
AC_DEFUN([AC_ENABLE_SHARED],
[define([AC_ENABLE_SHARED_DEFAULT], ifelse($1, no, no, yes))dnl
AC_ARG_ENABLE(shared,
changequote(<<, >>)dnl
<<  --enable-shared[=PKGS]    build shared libraries [default=>>AC_ENABLE_SHARED_DEFAULT],
changequote([, ])dnl
[p=${PACKAGE-default}
case $enableval in
yes) enable_shared=yes ;;
no) enable_shared=no ;;
*)
  enable_shared=no
  # Look at the argument we got.  We use all the common list separators.
  IFS="${IFS=   }"; ac_save_ifs="$IFS"; IFS="${IFS}:,"
  for pkg in $enableval; do
    if test "X$pkg" = "X$p"; then
      enable_shared=yes
    fi

  done
  IFS="$ac_save_ifs"
  ;;
esac],
enable_shared=AC_ENABLE_SHARED_DEFAULT)dnl
])

dnl AC_ENABLE_STATIC - implement the --enable-static flag
dnl Usage: AC_ENABLE_STATIC[(DEFAULT)]
dnl   Where DEFAULT is either `yes' or `no'.  If omitted, it defaults to
dnl   `yes'.
AC_DEFUN([AC_ENABLE_STATIC],
[define([AC_ENABLE_STATIC_DEFAULT], ifelse($1, no, no, yes))dnl
AC_ARG_ENABLE(static,
changequote(<<, >>)dnl
<<  --enable-static[=PKGS]    build static libraries [default=>>AC_ENABLE_STATIC_DEFAULT],
changequote([, ])dnl
[p=${PACKAGE-default}
case $enableval in
yes) enable_static=yes ;;
no) enable_static=no ;;
*)
  enable_static=no
  # Look at the argument we got.  We use all the common list separators.
  IFS="${IFS=   }"; ac_save_ifs="$IFS"; IFS="${IFS}:,"
  for pkg in $enableval; do
    if test "X$pkg" = "X$p"; then
      enable_static=yes
    fi
  done
  IFS="$ac_save_ifs"
  ;;
esac],
enable_static=AC_ENABLE_STATIC_DEFAULT)dnl
])

dnl AC_DISABLE_STATIC - set the default static flag to --disable-static
AC_DEFUN([AC_DISABLE_STATIC],
[AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
AC_ENABLE_STATIC(no)])

dnl AC_TRY_RUN_STRICT(PROGRAM,CFLAGS,CPPFLAGS,LDFLAGS,
dnl		[ACTION-IF-TRUE],[ACTION-IF-FALSE],
dnl		[ACTION-IF-CROSS-COMPILING = RUNTIME-ERROR])
AC_DEFUN( [AC_TRY_RUN_STRICT],
[
	old_CFLAGS="$CFLAGS";
	CFLAGS="$2";
	export CFLAGS;
	old_CPPFLAGS="$CPPFLAGS";
	CPPFLAGS="$3";
	export CPPFLAGS;
	old_LDFLAGS="$LDFLAGS";
	LDFLAGS="$4";
	export LDFLAGS;
	AC_TRY_RUN([$1],[$5],[$6],[$7])
	CFLAGS="$old_CFLAGS";
	old_CFLAGS="";
	export CFLAGS;
	CPPFLAGS="$old_CPPFLAGS";
	old_CPPFLAGS="";
	export CPPFLAGS;
	LDFLAGS="$old_LDFLAGS";
	old_LDFLAGS="";
	export LDFLAGS;
])

dnl SMB_CHECK_SYSCONF(varname)
dnl Tests whether the sysconf(3) variable "varname" is available.
AC_DEFUN([SMB_CHECK_SYSCONF],
[
    AC_CACHE_CHECK([for sysconf($1)],
	samba_cv_SYSCONF$1,
	[
	    AC_TRY_LINK([#include <unistd.h>],
		[ return sysconf($1) == -1 ? 1 : 0; ],
		[ samba_cv_SYSCONF$1=yes ],
		[ samba_cv_SYSCONF$1=no ])
	])

    if test x"$samba_cv_SYSCONF$1" = x"yes" ; then
	AC_DEFINE(SYSCONF$1, 1, [Whether sysconf($1) is available])
    fi
])

dnl SMB_IS_LIBPTHREAD_LINKED([actions if true], [actions if false])
dnl Test whether the current LIBS results in libpthread being present.
dnl Execute the corresponding user action list.
AC_DEFUN([SMB_IS_LIBPTHREAD_LINKED],
[
    AC_MSG_CHECKING(if libpthread is linked)
    AC_TRY_LINK([],
	[return pthread_create(0, 0, 0, 0);],
	[
	    AC_MSG_RESULT(yes)
	    $1
	],
	[
	    AC_MSG_RESULT(no)
	    $2
	])
])

dnl SMB_REMOVE_LIB(lib)
dnl Remove the given library from $LIBS
AC_DEFUN([SMB_REMOVELIB],
[
    LIBS=`echo $LIBS | sed -es/-l$1//g`
])

dnl SMB_CHECK_DMAPI([actions if true], [actions if false])
dnl Check whether DMAPI is available and is a version that we know
dnl how to deal with. The default truth action is to set samba_dmapi_libs
dnl to the list of necessary libraries, and to define USE_DMAPI.
AC_DEFUN([SMB_CHECK_DMAPI],
[
    samba_dmapi_libs=""

    if test x"$samba_dmapi_libs" = x"" ; then
	AC_CHECK_LIB(dm, dm_get_eventlist,
		[ samba_dmapi_libs="-ldm"], [])
    fi

    if test x"$samba_dmapi_libs" = x"" ; then
	AC_CHECK_LIB(jfsdm, dm_get_eventlist,
		[samba_dmapi_libs="-ljfsdm"], [])
    fi

    if test x"$samba_dmapi_libs" = x"" ; then
	AC_CHECK_LIB(xdsm, dm_get_eventlist,
		[samba_dmapi_libs="-lxdsm"], [])
    fi

    # Only bother to test ehaders if we have a candidate DMAPI library
    if test x"$samba_dmapi_libs" != x"" ; then
	AC_CHECK_HEADERS(sys/dmi.h xfs/dmapi.h sys/jfsdmapi.h sys/dmapi.h)
    fi

    if test x"$samba_dmapi_libs" != x"" ; then
	samba_dmapi_save_LIBS="$LIBS"
	LIBS="$LIBS $samba_dmapi_libs"
	AC_TRY_LINK(
		[
#include <time.h>      /* needed by Tru64 */
#include <sys/types.h> /* needed by AIX */
#ifdef HAVE_XFS_DMAPI_H
#include <xfs/dmapi.h>
#elif defined(HAVE_SYS_DMI_H)
#include <sys/dmi.h>
#elif defined(HAVE_SYS_JFSDMAPI_H)
#include <sys/jfsdmapi.h>
#elif defined(HAVE_SYS_DMAPI_H)
#include <sys/dmapi.h>
#endif
		],
		[
/* This link test is designed to fail on IRI 6.4, but should
 * succeed on Linux, IRIX 6.5 and AIX.
 */
	char * version;
	dm_eventset_t events;
	/* This doesn't take an argument on IRIX 6.4. */
	dm_init_service(&version);
	/* IRIX 6.4 expects events to be a pointer. */
	DMEV_ISSET(DM_EVENT_READ, events);
		],
		[
		    true # DMAPI link test succeeded
		],
		[
		    # DMAPI link failure
		    samba_dmapi_libs=
		])
	LIBS="$samba_dmapi_save_LIBS"
    fi

    if test x"$samba_dmapi_libs" = x"" ; then
	# DMAPI detection failure actions begin
	ifelse($2, [],
	    [
		AC_ERROR(Failed to detect a supported DMAPI implementation)
	    ],
	    [
		$2
	    ])
	# DMAPI detection failure actions end
    else
	# DMAPI detection success actions start
	ifelse($1, [],
	    [
		AC_DEFINE(USE_DMAPI, 1,
		    [Whether we should build DMAPI integration components])
		AC_MSG_NOTICE(Found DMAPI support in $samba_dmapi_libs)
	    ],
	    [
		$1
	    ])
	# DMAPI detection success actions end
    fi

])

dnl SMB_CHECK_CLOCK_ID(clockid)
dnl Test whether the specified clock_gettime clock ID is available. If it
dnl is, we define HAVE_clockid
AC_DEFUN([SMB_CHECK_CLOCK_ID],
[
    AC_MSG_CHECKING(for $1)
    AC_TRY_LINK([
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
    ],
    [
clockid_t clk = $1;
    ],
    [
	AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_$1, 1,
	    [Whether the clock_gettime clock ID $1 is available])
    ],
    [
	AC_MSG_RESULT(no)
    ])
])
