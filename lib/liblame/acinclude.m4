dnl acinclude.m4. Change *this* file to add new or change macros.
dnl When changes have been made, delete aclocal.m4 and run
dnl "aclocal".
dnl
dnl DO NOT change aclocal.m4 !
dnl

dnl * LA_SEARCH_FILE(variable, filename, PATH)
dnl * Search "filename" in the specified "PATH", "variable" will 
dnl * contain the full pathname or the empty string
dnl * PATH is space-separated list of directories.
dnl * by Florian Bomers

AC_DEFUN([LA_SEARCH_FILE],[
  $1=
  dnl hack: eliminate line feeds in $2
  for FILE in $2; do
    for DIR in $3; do
      dnl use PATH in order
      if test ".$1"="." && test -f "$DIR/$FILE"; then
        $1=$DIR
      fi
    done
  done
])

dnl * LA_SEARCH_LIB(lib-variable, include-variable, lib-filename, header-filename, prefix)
dnl * looks for "lib-filename" and "header-filename" in the area of "prefix".
dnl * if found, "lib-variable" and "include-variable" are set to the
dnl * respective paths.
dnl * prefix is a single path
dnl * libs are searched in prefix, prefix/lib, prefix/.., prefix/../lib
dnl * headers are searched in prefix, prefix/include, prefix/.., prefix/../include
dnl * 
dnl * If one of them is not found, both "lib-variable", "include-variable" are 
dnl * set to the empty string.
dnl *
dnl * TODO: assert function call to verify lib
dnl *
dnl * by Florian Bomers

AC_DEFUN([LA_SEARCH_LIB],[
  dnl look for lib
  LA_SEARCH_FILE($1, $3, $5 $5/lib $5/.. $5/../lib)
  dnl look for header.
  LA_SEARCH_FILE($2, $4, $5 $5/include $5/.. $5/../include)
  if test ".$1" = "." || test ".$2" = "."; then
    $1=
    $2=
  fi
])


 

# alex_IEEE854_FLOAT80
# ------------
AC_DEFUN([alex_IEEE854_FLOAT80],
[AC_CACHE_CHECK(for IEEE854 compliant 80 bit floats, alex_cv_ieee854_float80,
[AC_TRY_RUN([
int   float2long_IEEE_compliance ( void )
{
    struct {
        long padding; /* to prevent unaligned access */
        float  f;
    } s;
    s.f = 12582912.; if ( *(long*)(&s.f) != 1262485504l ) return 0;
    s.f = 12615679.; if ( *(long*)(&s.f) != 1262518271l ) return 0;
    s.f = 13582912.; if ( *(long*)(&s.f) != 1263485504l ) return 0;
    s.f = 12550145.; if ( *(long*)(&s.f) != 1262452737l ) return 0;
    s.f = 11582912.; if ( *(long*)(&s.f) != 1261485504l ) return 0;
    return 1;
}

int main(void)
{
    int retval;

    retval = float2long_IEEE_compliance();

    /* no error return -> success */
    return !retval;
}
], alex_cv_ieee854_float80=yes, alex_cv_ieee854_float80=no,
[AC_MSG_WARN(can't check for IEEE854 compliant 80 bit floats)]
)])]) # alex_IEEE854_FLOAT80


