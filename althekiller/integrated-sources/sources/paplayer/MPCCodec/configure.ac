dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/mpc_decoder.c)
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(libmpcdec,1.2.2)
AM_CONFIG_HEADER(include/config.h)

AM_PROG_LIBTOOL

CFLAGS="$CFLAGS -O3 -fomit-frame-pointer -fPIC"

AC_C_BIGENDIAN(,CFLAGS="$CFLAGS -DMPC_LITTLE_ENDIAN",)

AC_HEADER_STDC
AC_CHECK_HEADERS([inttypes.h string.h])

AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])

AC_FUNC_MEMCMP
AC_CHECK_FUNCS([memmove memset sqrt])

AC_MSG_CHECKING(for int16_t)
AC_CACHE_VAL(has_int16_t,
[AC_TRY_RUN([
int16_t foo;
int main() {return 0;}
],
has_int16_t=yes,
has_int16_t=no,
has_int16_t=no
)])
AC_MSG_RESULT($has_int16_t)

AC_MSG_CHECKING(for int32_t)
AC_CACHE_VAL(has_int32_t,
[AC_TRY_RUN([
int32_t foo;
int main() {return 0;}
],
has_int32_t=yes,
has_int32_t=no,
has_int32_t=no
)])
AC_MSG_RESULT($has_int32_t)

AC_MSG_CHECKING(for uint32_t)
AC_CACHE_VAL(has_uint32_t,
[AC_TRY_RUN([
uint32_t foo;
int main() {return 0;}
],
has_uint32_t=yes,
has_uint32_t=no,
has_uint32_t=no
)])
AC_MSG_RESULT($has_uint32_t)

AC_MSG_CHECKING(for uint16_t)
AC_CACHE_VAL(has_uint16_t,
[AC_TRY_RUN([
uint16_t foo;
int main() {return 0;}
],
has_uint16_t=yes,
has_uint16_t=no,
has_uint16_t=no
)])
AC_MSG_RESULT($has_uint16_t)

AC_MSG_CHECKING(for u_int32_t)
AC_CACHE_VAL(has_u_int32_t,
[AC_TRY_RUN([
u_int32_t foo;
int main() {return 0;}
],
has_u_int32_t=yes,
has_u_int32_t=no,
has_u_int32_t=no
)])
AC_MSG_RESULT($has_u_int32_t)

AC_MSG_CHECKING(for u_int16_t)
AC_CACHE_VAL(has_u_int16_t,
[AC_TRY_RUN([
u_int16_t foo;
int main() {return 0;}
],
has_u_int16_t=yes,
has_u_int16_t=no,
has_u_int16_t=no
)])
AC_MSG_RESULT($has_u_int16_t)

AC_MSG_CHECKING(for int64_t)
AC_CACHE_VAL(has_int64_t,
[AC_TRY_RUN([
int64_t foo;
int main() {return 0;}
],
has_int64_t=yes,
has_int64_t=no,
has_int64_t=no
)])
AC_MSG_RESULT($has_int64_t)

AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)


if test x$has_int16_t = "xyes" ; then
        SIZE16="int16_t"
else
        case 2 in
                $ac_cv_sizeof_short) SIZE16="short";;
                $ac_cv_sizeof_int) SIZE16="int";;
        esac
fi

if test x$has_int32_t = "xyes" ; then
        SIZE32="int32_t"
else
        case 4 in
                $ac_cv_sizeof_short) SIZE32="short";;
                $ac_cv_sizeof_int) SIZE32="int";;
                $ac_cv_sizeof_long) SIZE32="long";;
        esac
fi

if test x$has_uint32_t = "xyes" ; then
        USIZE32="uint32_t"
else
        if test x$has_u_int32_t = "xyes" ; then
                USIZE32="u_int32_t"
        else
                case 4 in
                        $ac_cv_sizeof_short) USIZE32="unsigned short";;
                        $ac_cv_sizeof_int) USIZE32="unsigned int";;
                        $ac_cv_sizeof_long) USIZE32="unsigned long";;
                esac
        fi
fi

if test x$has_uint16_t = "xyes" ; then
        USIZE16="uint16_t"
else
        if test x$has_u_int16_t = "xyes" ; then
                USIZE16="u_int16_t"
        else
                case 2 in
                        $ac_cv_sizeof_short) USIZE16="unsigned short";;
                        $ac_cv_sizeof_int) USIZE16="unsigned int";;
                        $ac_cv_sizeof_long) USIZE16="unsigned long";;
                esac
        fi
fi

if test x$has_int64_t = "xyes" ; then
        SIZE64="int64_t"
else
case 8 in
        $ac_cv_sizeof_int) SIZE64="int";;
        $ac_cv_sizeof_long) SIZE64="long";;
        $ac_cv_sizeof_long_long) SIZE64="long long";;
esac
fi

if test -z "$SIZE16"; then
        AC_MSG_ERROR(No 16 bit type found on this platform!)
fi
if test -z "$USIZE16"; then
        AC_MSG_ERROR(No unsigned 16 bit type found on this platform!)
fi
if test -z "$SIZE32"; then
        AC_MSG_ERROR(No 32 bit type found on this platform!)
fi
if test -z "$USIZE32"; then
        AC_MSG_ERROR(No unsigned 32 bit type found on this platform!)
fi
if test -z "$SIZE64"; then
        AC_MSG_WARN(No 64 bit type found on this platform!)
fi

dnl Make substitutions

AC_SUBST(VERSION)
AC_SUBST(LIBTOOL_DEPS)
AC_SUBST(SIZE16)
AC_SUBST(USIZE16)
AC_SUBST(SIZE32)
AC_SUBST(USIZE32)
AC_SUBST(SIZE64)
AC_SUBST(OPT)
AC_SUBST(LIBS)

AC_OUTPUT(Makefile src/Makefile include/Makefile include/mpcdec/config_types.h)
AC_MSG_RESULT([=> libmpcdec $VERSION configured successfully])
