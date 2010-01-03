AC_INIT(configure.ac)
AC_CHECK_TOOL(LD, ld, :)

AC_ARG_WITH(kernel-path, [  --with-kernel-path    Specify kernel path],
    KERN_DIR=$withval, KERN_DIR="/lib/modules/"$(uname -r)"/build")


CRYSTALHDROOT="$(pwd)"

AC_SUBST(CRYSTALHDROOT)
AC_SUBST(KERN_DIR)

AC_OUTPUT([
./Makefile
])
