#!/bin/sh
#
# Copyright (C) Nalin Dahyabhai <nalin@redhat.com> 2003
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

tempdir=`mktemp -d /tmp/dlopenXXXXXX`
test -n "$tempdir" || exit 1
cat >> $tempdir/dlopen.c << _EOF
#include <dlfcn.h>
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
/* Simple program to see if dlopen() would succeed. */
int main(int argc, char **argv)
{
	int i;
	struct stat st;
	char buf[PATH_MAX];
	for (i = 1; i < argc; i++) {
		if (dlopen(argv[i], RTLD_NOW)) {
			fprintf(stdout, "dlopen() of \"%s\" succeeded.\n",
				argv[i]);
		} else {
			snprintf(buf, sizeof(buf), "./%s", argv[i]);
			if ((stat(buf, &st) == 0) && dlopen(buf, RTLD_NOW)) {
				fprintf(stdout, "dlopen() of \"./%s\" "
					"succeeded.\n", argv[i]);
			} else {
				fprintf(stdout, "dlopen() of \"%s\" failed: "
					"%s\n", argv[i], dlerror());
				return 1;
			}
		}
	}
	return 0;
}
_EOF

for arg in $@ ; do
	case "$arg" in
	"")
		;;
	-I*|-D*|-f*|-m*|-g*|-O*|-W*)
		cflags="$cflags $arg"
		;;
	-l*|-L*)
		ldflags="$ldflags $arg"
		;;
	/*)
		modules="$modules $arg"
		;;
	*)
		modules="$modules $arg"
		;;
	esac
done

${CC:-gcc} $RPM_OPT_FLAGS $CFLAGS -o $tempdir/dlopen $cflags $tempdir/dlopen.c $ldflags -ldl

retval=0
for module in $modules ; do
	case "$module" in
	"")
		;;
	/*)
		$tempdir/dlopen "$module"
		retval=$?
		;;
	*)
		$tempdir/dlopen ./"$module"
		retval=$?
		;;
	esac
done

rm -f $tempdir/dlopen $tempdir/dlopen.c
rmdir $tempdir
exit $retval
