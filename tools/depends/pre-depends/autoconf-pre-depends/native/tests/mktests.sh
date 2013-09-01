#! /bin/sh

# Build some of the Autoconf test files.

# Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
# 2009, 2010 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# If we fail, clean up, but touch the output files.  We probably failed
# because we used some non-portable tool.

as_me=`echo "$0" | sed 's|.*[\\/]||'`

trap 'echo "'"$as_me"': failed." >&2
      rm -f acdefuns audefuns requires *.tat
      trap "" 0
      exit 1' \
     0 1 2 15

# If ever something goes wrong, fail, so that the trap is launched.
set -e

# We need arguments.
test $# != 0

# We need these arguments.
src="$@"

# Set locale to C so that `sort' behaves in a uniform way.
LANGUAGE=C; export LANGUAGE
LANG=C; export LANG
LC_ALL=C export LC_ALL


# requires
# --------
# Get the list of macros that are required: there is little interest
# in testing them since they will be run by the guy who requires them.
sed -n 's/dnl.*//;s/.*AC_REQUIRE(\[*\([a-zA-Z0-9_]*\).*$/\1/p' $src |
  sort -u >requires


# exclude_list
# ------------
# Macros which must not be checked at all (not with AT_CHECK_MACRO nor
# AT_CHECK_AU_MACRO).
exclude_list='
	# Not a macro name at all.
	/^$/ {next}

	# Not macros, just mapping from old variable name to a new one.
	/^ac_cv_prog_(gcc|gxx|g77)$/ {next}
'


# ac_exclude_list
# ---------------
# We try to test all the Autoconf macros with AT_CHECK_MACRO to check
# for syntax problems, etc.  Not every macros can be run without
# arguments, and some are already tested elsewhere.  AC_EXCLUDE_LIST
# filters out the macros we don't want to test.
ac_exclude_list='
	# Internal macros are used elsewhere.
	/^_?_AC_/ {next}

	# Used in many places.
	/^AC_.*_IFELSE$/ {next}
	/^AC_LANG/ {next}
	/^AC_RUN_LOG$/ {next}
	/^AC_TRY/ {next}

	# Need an argument.
	/^AC_(CANONICALIZE|PREFIX_PROGRAM|PREREQ)$/ {next}
	/^AC_(SEARCH_LIBS|REPLACE_FUNCS)$/ {next}
	/^AC_(CACHE_CHECK|COMPUTE)_INT$/ {next}
	/^AC_ARG_VAR$/ {next}
	/^AC_REQUIRE_SHELL_FN$/ {next}

	# Performed in the semantics tests.
	/^AC_CHECK_(ALIGNOF|DECL|FILE|FUNC|HEADER|LIB|MEMBER|PROG|SIZEOF|(TARGET_)?TOOL|TYPE)S?$/ {next}
	/^AC_PATH_PROGS_FEATURE_CHECK$/ {next}

	# Fail when the source does not exist.
	/^AC_CONFIG/ {next}

	# AU defined to nothing.
	/^AC_(CYGWIN|CYGWIN32|EMXOS2|MING32|EXEEXT|OBJEXT)$/ {next}

	# Produce "= val" because $1, the variable used to store the result,
	# is empty.
	/^AC_(F77|FC)_FUNC$/ {next}
	/^AC_FC_SRCEXT$/ {next}
	/^AC_PATH_((TARGET_)?TOOL|PROG)S?$/ {next}

	# Is a number.
	/^AC_FD_CC$/ {next}

	# Obsolete, but needs to be AC_DEFUNed.
	/^AC_FOREACH$/ {next}

	# Require a file that is not shipped with Autoconf.  But it should.
	/^AC_FUNC_(GETLOADAVG|FNMATCH_GNU)$/ {next}
	/^AC_REPLACE_FNMATCH$/ {next}

	# Obsolete, checked in semantics.
	/^AC_FUNC_WAIT3$/ {next}
	/^AC_FUNC_SETVBUF_REVERSED$/ {next}
	/^AC_SYS_RESTARTABLE_SYSCALLS$/ {next}

	# Not intended to be invoked at the top level.
	/^AC_INCLUDES_DEFAULT$/ {next}

	# AC_INIT includes all the AC_INIT macros.
	# There is an infinite m4 recursion if AC_INIT is used twice.
	/^AC_INIT/ {next}

	# Checked in semantics.
	/^AC_(PROG_CC|C_CONST|C_VOLATILE)$/ {next}
	/^AC_PATH_XTRA$/ {next}

	# Requires a working C++ compiler, which is not a given.
	/^AC_PROG_CXX_C_O$/ {next}

	# Already tested by AT_CHECK_MACRO.
	/^AC_OUTPUT$/ {next}

	# Tested alongside m4_divert_text.
	/^AC_PRESERVE_HELP_ORDER$/ {next}
'


# ac_exclude_script
# -----------------
# Build a single awk script out of the above.
ac_exclude_script="$exclude_list $ac_exclude_list {print}"


# au_exclude_list
# ---------------
# Check all AU_DEFUN'ed macros with AT_CHECK_AU_MACRO, except these.
au_exclude_list='
	# Empty.
	/^AC_(C_CROSS|PROG_CC_STDC)$/ {next}

	# Use AC_REQUIRE.
	/^AC_(CYGWIN|MINGW32|EMXOS2)$/ {next}

	# Already in configure.ac.
	/^AC_(INIT|OUTPUT)$/ {next}

	# AC_LANG_SAVE needs user interaction to be removed.
	# AC_LANG_RESTORE cannot be used alone.
	/^AC_LANG_(SAVE|RESTORE)$/ {next}

	# Need arguments and are tested elsewhere.
	/^AC_(LINK_FILES|PREREQ)$/ {next}
'

# au_exclude_script
# -----------------
# Build a single awk script out of the above.
au_exclude_script="$exclude_list $au_exclude_list {print}"


## ------------------------- ##
## Creating the test files.  ##
## ------------------------- ##

for file in $src
do
  base=`echo "$file" | sed 's|.*[\\/]||;s|\..*||'`
  # Get the list of macros which are defined in Autoconf level.
  # Get rid of the macros we are not interested in.
  sed -n -e 's/^AC_DEFUN(\[*\([a-zA-Z0-9_]*\).*$/\1/p' \
	 -e 's/^AC_DEFUN_ONCE(\[*\([a-zA-Z0-9_]*\).*$/\1/p' $file |
    awk "$ac_exclude_script" |
    sort -u >acdefuns

  # Get the list of macros which are defined in Autoupdate level.
  sed -n 's/^AU_DEFUN(\[*\([a-zA-Z][a-zA-Z0-9_]*\).*$/\1/p' $file |
    awk "$au_exclude_script" |
    sort -u >audefuns

  # Filter out required macros.
  {
    sed 's/^ *//' <<MK_EOF
    # Generated by $as_me.			-*- Autotest -*-

    ## --------------------- ##
    ## Do not edit by hand.  ##
    ## --------------------- ##

    # Copyright (C) 2000, 2001, 2003, 2004, 2005, 2006, 2007, 2008, 2009
    # 2010 Free Software Foundation, Inc.

    AT_BANNER([Testing autoconf/$base macros.])

MK_EOF

    echo "# Modern macros."
    comm -23 acdefuns requires | sed 's/.*/AT_CHECK_MACRO([&])/'
    echo
    echo "# Obsolete macros."
    comm -23 audefuns requires | sed 's/.*/AT_CHECK_AU_MACRO([&])/'
  } >ac$base.tat

  # In one atomic step so that if something above fails, the trap
  # preserves the old version of the file.  If there is nothing to
  # check, output /rien du tout/[1].
  if grep AT_CHECK ac$base.tat >/dev/null 2>&1; then
    mv -f ac$base.tat ac$base.at
  else
    rm -f ac$base.tat ac$base.at
    touch ac$base.at
  fi
  # Help people not to update these files by hand.
  chmod a-w ac$base.at
done

rm -f acdefuns audefuns requires

trap '' 0
exit 0

# [1] En franc,ais dans le texte.
