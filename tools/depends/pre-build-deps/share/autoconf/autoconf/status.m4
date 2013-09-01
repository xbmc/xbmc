# This file is part of Autoconf.                       -*- Autoconf -*-
# Parameterizing and creating config.status.
# Copyright (C) 1992, 1993, 1994, 1995, 1996, 1998, 1999, 2000, 2001,
# 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010 Free Software
# Foundation, Inc.

# This file is part of Autoconf.  This program is free
# software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# Under Section 7 of GPL version 3, you are granted additional
# permissions described in the Autoconf Configure Script Exception,
# version 3.0, as published by the Free Software Foundation.
#
# You should have received a copy of the GNU General Public License
# and a copy of the Autoconf Configure Script Exception along with
# this program; see the files COPYINGv3 and COPYING.EXCEPTION
# respectively.  If not, see <http://www.gnu.org/licenses/>.


# Written by David MacKenzie, with help from
# Franc,ois Pinard, Karl Berry, Richard Pixley, Ian Lance Taylor,
# Roland McGrath, Noah Friedman, david d zuhn, and many others.


# This file handles about all the preparation aspects for
# `config.status': registering the configuration files, the headers,
# the links, and the commands `config.status' will run.  There is a
# little mixture though of things actually handled by `configure',
# such as running the `configure' in the sub directories.  Minor
# detail.
#
# There are two kinds of commands:
#
# COMMANDS:
#
#   They are output into `config.status' via a quoted here doc.  These
#   commands are always associated to a tag which the user can use to
#   tell `config.status' what are the commands she wants to run.
#
# INIT-CMDS:
#
#   They are output via an *unquoted* here-doc.  As a consequence $var
#   will be output as the value of VAR.  This is typically used by
#   `configure' to give `config.status' some variables it needs to run
#   the COMMANDS.  At the difference of COMMANDS, the INIT-CMDS are
#   always run.
#
#
# Honorable members of this family are AC_CONFIG_FILES,
# AC_CONFIG_HEADERS, AC_CONFIG_LINKS and AC_CONFIG_COMMANDS.  Bad boys
# are AC_LINK_FILES, AC_OUTPUT_COMMANDS and AC_OUTPUT when used with
# arguments.  False members are AC_CONFIG_SRCDIR, AC_CONFIG_SUBDIRS
# and AC_CONFIG_AUX_DIR.  Cousins are AC_CONFIG_COMMANDS_PRE and
# AC_CONFIG_COMMANDS_POST.


## ------------------ ##
## Auxiliary macros.  ##
## ------------------ ##

# _AC_SRCDIRS(BUILD-DIR-NAME)
# ---------------------------
# Inputs:
#   - BUILD-DIR-NAME is `top-build -> build' and `top-src -> src'
#   - `$srcdir' is `top-build -> top-src'
#
# Outputs:
# - `ac_builddir' is `.', for symmetry only.
# - `ac_top_builddir_sub' is `build -> top_build'.
#      This is used for @top_builddir@.
# - `ac_top_build_prefix' is `build -> top_build'.
#      If not empty, has a trailing slash.
# - `ac_srcdir' is `build -> src'.
# - `ac_top_srcdir' is `build -> top-src'.
# and `ac_abs_builddir' etc., the absolute directory names.
m4_define([_AC_SRCDIRS],
[ac_builddir=.

case $1 in
.) ac_dir_suffix= ac_top_builddir_sub=. ac_top_build_prefix= ;;
*)
  ac_dir_suffix=/`AS_ECHO([$1]) | sed 's|^\.[[\\/]]||'`
  # A ".." for each directory in $ac_dir_suffix.
  ac_top_builddir_sub=`AS_ECHO(["$ac_dir_suffix"]) | sed 's|/[[^\\/]]*|/..|g;s|/||'`
  case $ac_top_builddir_sub in
  "") ac_top_builddir_sub=. ac_top_build_prefix= ;;
  *)  ac_top_build_prefix=$ac_top_builddir_sub/ ;;
  esac ;;
esac
ac_abs_top_builddir=$ac_pwd
ac_abs_builddir=$ac_pwd$ac_dir_suffix
# for backward compatibility:
ac_top_builddir=$ac_top_build_prefix

case $srcdir in
  .)  # We are building in place.
    ac_srcdir=.
    ac_top_srcdir=$ac_top_builddir_sub
    ac_abs_top_srcdir=$ac_pwd ;;
  [[\\/]]* | ?:[[\\/]]* )  # Absolute name.
    ac_srcdir=$srcdir$ac_dir_suffix;
    ac_top_srcdir=$srcdir
    ac_abs_top_srcdir=$srcdir ;;
  *) # Relative name.
    ac_srcdir=$ac_top_build_prefix$srcdir$ac_dir_suffix
    ac_top_srcdir=$ac_top_build_prefix$srcdir
    ac_abs_top_srcdir=$ac_pwd/$srcdir ;;
esac
ac_abs_srcdir=$ac_abs_top_srcdir$ac_dir_suffix
])# _AC_SRCDIRS


# _AC_HAVE_TOP_BUILD_PREFIX
# -------------------------
# Announce to the world (to Libtool) that we substitute @top_build_prefix@.
AC_DEFUN([_AC_HAVE_TOP_BUILD_PREFIX])


## ---------------------- ##
## Registering the tags.  ##
## ---------------------- ##


# _AC_CONFIG_COMMANDS_INIT([INIT-COMMANDS])
# -----------------------------------------
#
# Register INIT-COMMANDS as command pasted *unquoted* in
# `config.status'.  This is typically used to pass variables from
# `configure' to `config.status'.  Note that $[1] is not over quoted as
# was the case in AC_OUTPUT_COMMANDS.
m4_define([_AC_CONFIG_COMMANDS_INIT],
[m4_ifval([$1],
	  [m4_append([_AC_OUTPUT_COMMANDS_INIT],
		     [$1
])])])


# AC_FILE_DEPENDENCY_TRACE(DEST, SOURCE1, [SOURCE2...])
# -----------------------------------------------------
# This macro does nothing, it's a hook to be read with `autoconf --trace'.
#
# It announces DEST depends upon the SOURCE1 etc.
m4_define([AC_FILE_DEPENDENCY_TRACE], [])


# _AC_FILE_DEPENDENCY_TRACE_COLON(DEST:SOURCE1[:SOURCE2...])
# ----------------------------------------------------------
# Declare that DEST depends upon SOURCE1 etc.
#
m4_define([_AC_FILE_DEPENDENCY_TRACE_COLON],
[AC_FILE_DEPENDENCY_TRACE(m4_translit([$1], [:], [,]))])


# _AC_CONFIG_DEPENDENCY(MODE, DEST[:SOURCE1...])
# ----------------------------------------------
# MODE is `FILES', `HEADERS', or `LINKS'.
#
# Be sure that a missing dependency is expressed as a dependency upon
# `DEST.in' (except with config links).
#
m4_define([_AC_CONFIG_DEPENDENCY],
[_AC_FILE_DEPENDENCY_TRACE_COLON([$2]_AC_CONFIG_DEPENDENCY_DEFAULT($@))dnl
])


# _AC_CONFIG_DEPENDENCY_DEFAULT(MODE, DEST[:SOURCE1...])
# ------------------------------------------------------
# Expand to `:DEST.in' if appropriate, or to empty string otherwise.
#
# More detailed description:
# If the tag contains `:', expand to nothing.
# Otherwise, for a config file or header, add `:DEST.in'.
# For a config link, DEST.in is not appropriate:
#  - if the tag is literal, complain.
#  - otherwise, just expand to nothing and proceed with fingers crossed.
#    (We get to this case from the obsolete AC_LINK_FILES, for example.)
#
m4_define([_AC_CONFIG_DEPENDENCY_DEFAULT],
[m4_if(m4_index([$2], [:]), [-1],
	   [m4_if([$1], [LINKS],
		  [AS_LITERAL_IF([$2],
		    [m4_fatal([Invalid AC_CONFIG_LINKS tag: `$2'])])],
		  [:$2.in])])])


# _AC_CONFIG_UNIQUE(MODE, DEST)
# -----------------------------
# MODE is `FILES', `HEADERS', `LINKS', `COMMANDS', or `SUBDIRS'.
#
# Verify that there is no double definition of an output file.
#
m4_define([_AC_CONFIG_UNIQUE],
[m4_ifdef([_AC_SEEN_TAG($2)],
   [m4_fatal([`$2' is already registered with AC_CONFIG_]m4_defn(
     [_AC_SEEN_TAG($2)]).)],
   [m4_define([_AC_SEEN_TAG($2)], [$1])])dnl
])


# _AC_CONFIG_FOOS(MODE, TAGS..., [COMMANDS], [INIT-CMDS])
# -------------------------------------------------------
# MODE is `FILES', `HEADERS', `LINKS', or `COMMANDS'.
#
# Associate the COMMANDS to each TAG, i.e., when config.status creates TAG,
# run COMMANDS afterwards.  (This is done in _AC_CONFIG_REGISTER_DEST.)
#
# For COMMANDS, do not m4_normalize TAGS before adding it to ac_config_commands.
# This historical difference allows macro calls in TAGS.
#
m4_define([_AC_CONFIG_FOOS],
[m4_map_args_w([$2], [_AC_CONFIG_REGISTER([$1],], [, [$3])])]dnl
[m4_define([_AC_SEEN_CONFIG(ANY)])]dnl
[m4_define([_AC_SEEN_CONFIG($1)])]dnl
[_AC_CONFIG_COMMANDS_INIT([$4])]dnl
[ac_config_[]m4_tolower([$1])="$ac_config_[]m4_tolower([$1]) ]dnl
[m4_if([$1], [COMMANDS], [$2], [m4_normalize([$2])])"
])

# _AC_CONFIG_COMPUTE_DEST(STRING)
# -------------------------------
# Compute the DEST from STRING by stripping any : and following
# characters.  Guarantee a match in m4_index, so as to avoid a bug
# with precision -1 in m4_format in older m4.
m4_define([_AC_CONFIG_COMPUTE_DEST],
[m4_format([[%.*s]], m4_index([$1:], [:]), [$1])])

# _AC_CONFIG_REGISTER(MODE, TAG, [COMMANDS])
# ------------------------------------------
# MODE is `FILES', `HEADERS', `LINKS', or `COMMANDS'.
#
m4_define([_AC_CONFIG_REGISTER],
[m4_if([$1], [COMMANDS],
       [],
       [_AC_CONFIG_DEPENDENCY([$1], [$2])])]dnl
[_AC_CONFIG_REGISTER_DEST([$1], [$2],
  _AC_CONFIG_COMPUTE_DEST([$2]), [$3])])


# _AC_CONFIG_REGISTER_DEST(MODE, TAG, DEST, [COMMANDS])
# -----------------------------------------------------
# MODE is `FILES', `HEADERS', `LINKS', or `COMMANDS'.
# TAG is in the form DEST[:SOURCE...].
# Thus parameter $3 is the first part of $2.
#
# With CONFIG_LINKS, reject DEST=., because it is makes it hard for ./config.status
# to guess the links to establish (`./config.status .').
#
# Save the name of the first config header to AH_HEADER.
#
m4_define([_AC_CONFIG_REGISTER_DEST],
[_AC_CONFIG_UNIQUE([$1], [$3])]dnl
[m4_if([$1 $3], [LINKS .],
       [m4_fatal([invalid destination of a config link: `.'])],
       [$1], [HEADERS],
       [m4_define_default([AH_HEADER], [$3])])]dnl
dnl
dnl Recognize TAG as an argument to config.status:
dnl
[m4_append([_AC_LIST_TAGS],
[    "$3") CONFIG_$1="$CONFIG_$1 $2" ;;
])]dnl
dnl
dnl Register the associated commands, if any:
dnl
[m4_ifval([$4],
[m4_append([_AC_LIST_TAG_COMMANDS],
[    "$3":]m4_format([[%.1s]], [$1])[) $4 ;;
])])])# _AC_CONFIG_REGISTER_DEST




## --------------------- ##
## Configuration files.  ##
## --------------------- ##


# AC_CONFIG_FILES(FILE..., [COMMANDS], [INIT-CMDS])
# -------------------------------------------------
# Specify output files, i.e., files that are configured with AC_SUBST.
#
AC_DEFUN([AC_CONFIG_FILES], [_AC_CONFIG_FOOS([FILES], $@)])


# _AC_SED_CMD_LIMIT
# -----------------
# Evaluate to an m4 number equal to the maximum number of commands to put
# in any single sed program, not counting ":" commands.
#
# Some seds have small command number limits, like on Digital OSF/1 and HP-UX.
m4_define([_AC_SED_CMD_LIMIT],
dnl One cannot portably go further than 99 commands because of HP-UX.
[99])


# _AC_AWK_LITERAL_LIMIT
# ---------------------
# Evaluate to the maximum number of characters to put in an awk
# string literal, not counting escape characters.
#
# Some awk's have small limits, such as Solaris and AIX awk.
m4_define([_AC_AWK_LITERAL_LIMIT],
[148])


# _AC_OUTPUT_FILES_PREPARE
# ------------------------
# Create the awk scripts needed for CONFIG_FILES.
# Support multiline substitutions and make sure that the substitutions are
# not evaluated recursively.
# The intention is to have readable config.status and configure, even
# though this m4 code might be scary.
#
# This code was written by Dan Manthey and rewritten by Ralf Wildenhues.
#
# This macro is expanded inside a here document.  If the here document is
# closed, it has to be reopened with
# "cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1".
#
m4_define([_AC_OUTPUT_FILES_PREPARE],
[# Set up the scripts for CONFIG_FILES section.
# No need to generate them if there are no CONFIG_FILES.
# This happens for instance with `./config.status config.h'.
if test -n "$CONFIG_FILES"; then

dnl For AC_SUBST_FILE, check for usable getline support in awk,
dnl at config.status execution time.
dnl Otherwise, do the interpolation in sh, which is slower.
dnl Without any AC_SUBST_FILE, omit all related code.
dnl Note the expansion is double-quoted for readability.
m4_ifdef([_AC_SUBST_FILES],
[[if $AWK 'BEGIN { getline <"/dev/null" }' </dev/null 2>/dev/null; then
  ac_cs_awk_getline=:
  ac_cs_awk_pipe_init=
  ac_cs_awk_read_file='
      while ((getline aline < (F[key])) > 0)
	print(aline)
      close(F[key])'
  ac_cs_awk_pipe_fini=
else
  ac_cs_awk_getline=false
  ac_cs_awk_pipe_init="print \"cat <<'|#_!!_#|' &&\""
  ac_cs_awk_read_file='
      print "|#_!!_#|"
      print "cat " F[key] " &&"
      '$ac_cs_awk_pipe_init
  # The final `:' finishes the AND list.
  ac_cs_awk_pipe_fini='END { print "|#_!!_#|"; print ":" }'
fi]])
ac_cr=`echo X | tr X '\015'`
# On cygwin, bash can eat \r inside `` if the user requested igncr.
# But we know of no other shell where ac_cr would be empty at this
# point, so we can use a bashism as a fallback.
if test "x$ac_cr" = x; then
  eval ac_cr=\$\'\\r\'
fi
ac_cs_awk_cr=`$AWK 'BEGIN { print "a\rb" }' </dev/null 2>/dev/null`
if test "$ac_cs_awk_cr" = "a${ac_cr}b"; then
  ac_cs_awk_cr='\\r'
else
  ac_cs_awk_cr=$ac_cr
fi
dnl
dnl Define the pipe that does the substitution.
m4_ifdef([_AC_SUBST_FILES],
[m4_define([_AC_SUBST_CMDS], [|
if $ac_cs_awk_getline; then
  $AWK -f "$ac_tmp/subs.awk"
else
  $AWK -f "$ac_tmp/subs.awk" | $SHELL
fi])],
[m4_define([_AC_SUBST_CMDS],
[| $AWK -f "$ac_tmp/subs.awk"])])dnl

echo 'BEGIN {' >"$ac_tmp/subs1.awk" &&
_ACEOF

m4_ifdef([_AC_SUBST_FILES],
[# Create commands to substitute file output variables.
{
  echo "cat >>$CONFIG_STATUS <<_ACEOF || ac_write_fail=1" &&
  echo 'cat >>"\$ac_tmp/subs1.awk" <<\\_ACAWK &&' &&
  echo "$ac_subst_files" | sed 's/.*/F@<:@"&"@:>@="$&"/' &&
  echo "_ACAWK" &&
  echo "_ACEOF"
} >conf$$files.sh &&
. ./conf$$files.sh ||
  AC_MSG_ERROR([could not make $CONFIG_STATUS])
rm -f conf$$files.sh
])dnl

{
  echo "cat >conf$$subs.awk <<_ACEOF" &&
  echo "$ac_subst_vars" | sed 's/.*/&!$&$ac_delim/' &&
  echo "_ACEOF"
} >conf$$subs.sh ||
  AC_MSG_ERROR([could not make $CONFIG_STATUS])
ac_delim_num=`echo "$ac_subst_vars" | grep -c '^'`
ac_delim='%!_!# '
for ac_last_try in false false false false false :; do
  . ./conf$$subs.sh ||
    AC_MSG_ERROR([could not make $CONFIG_STATUS])

dnl Do not use grep on conf$$subs.awk, since AIX grep has a line length limit.
  ac_delim_n=`sed -n "s/.*$ac_delim\$/X/p" conf$$subs.awk | grep -c X`
  if test $ac_delim_n = $ac_delim_num; then
    break
  elif $ac_last_try; then
    AC_MSG_ERROR([could not make $CONFIG_STATUS])
  else
    ac_delim="$ac_delim!$ac_delim _$ac_delim!! "
  fi
done
rm -f conf$$subs.sh

dnl Initialize an awk array of substitutions, keyed by variable name.
dnl
dnl The initial line contains the variable name VAR, then a `!'.
dnl Construct `S["VAR"]=' from it.
dnl The rest of the line, and potentially further lines, contain the
dnl substituted value; the last of those ends with $ac_delim.  We split
dnl the output both along those substituted newlines and at intervals of
dnl length _AC_AWK_LITERAL_LIMIT.  The latter is done to comply with awk
dnl string literal limitations, the former for simplicity in doing so.
dnl
dnl We deal with one input line at a time to avoid sed pattern space
dnl limitations.  We kill the delimiter $ac_delim before splitting the
dnl string (otherwise we risk splitting the delimiter).  And we do the
dnl splitting before the quoting of awk special characters (otherwise we
dnl risk splitting an escape sequence).
dnl
dnl Output as separate string literals, joined with backslash-newline.
dnl Eliminate the newline after `=' in a second script, for readability.
dnl
dnl Notes to the main part of the awk script:
dnl - the unusual FS value helps prevent running into the limit of 99 fields,
dnl - we avoid sub/gsub because of the \& quoting issues, see
dnl   http://www.gnu.org/software/gawk/manual/html_node/Gory-Details.html
dnl - Writing `$ 0' prevents expansion by both the shell and m4 here.
dnl
dnl m4-double-quote most of the scripting for readability.
[cat >>$CONFIG_STATUS <<_ACEOF || ac_write_fail=1
cat >>"\$ac_tmp/subs1.awk" <<\\_ACAWK &&
_ACEOF
sed -n '
h
s/^/S["/; s/!.*/"]=/
p
g
s/^[^!]*!//
:repl
t repl
s/'"$ac_delim"'$//
t delim
:nl
h
s/\(.\{]_AC_AWK_LITERAL_LIMIT[\}\)..*/\1/
t more1
s/["\\]/\\&/g; s/^/"/; s/$/\\n"\\/
p
n
b repl
:more1
s/["\\]/\\&/g; s/^/"/; s/$/"\\/
p
g
s/.\{]_AC_AWK_LITERAL_LIMIT[\}//
t nl
:delim
h
s/\(.\{]_AC_AWK_LITERAL_LIMIT[\}\)..*/\1/
t more2
s/["\\]/\\&/g; s/^/"/; s/$/"/
p
b
:more2
s/["\\]/\\&/g; s/^/"/; s/$/"\\/
p
g
s/.\{]_AC_AWK_LITERAL_LIMIT[\}//
t delim
' <conf$$subs.awk | sed '
/^[^""]/{
  N
  s/\n//
}
' >>$CONFIG_STATUS || ac_write_fail=1
rm -f conf$$subs.awk
cat >>$CONFIG_STATUS <<_ACEOF || ac_write_fail=1
_ACAWK
cat >>"\$ac_tmp/subs1.awk" <<_ACAWK &&
  for (key in S) S_is_set[key] = 1
  FS = ""
]m4_ifdef([_AC_SUBST_FILES],
[  \$ac_cs_awk_pipe_init])[
}
{
  line = $ 0
  nfields = split(line, field, "@")
  substed = 0
  len = length(field[1])
  for (i = 2; i < nfields; i++) {
    key = field[i]
    keylen = length(key)
    if (S_is_set[key]) {
      value = S[key]
      line = substr(line, 1, len) "" value "" substr(line, len + keylen + 3)
      len += length(value) + length(field[++i])
      substed = 1
    } else
      len += 1 + keylen
  }
]m4_ifdef([_AC_SUBST_FILES],
[[  if (nfields == 3 && !substed) {
    key = field[2]
    if (F[key] != "" && line ~ /^[	 ]*@.*@[	 ]*$/) {
      \$ac_cs_awk_read_file
      next
    }
  }]])[
  print line
}
]dnl end of double-quoted part
m4_ifdef([_AC_SUBST_FILES],
[\$ac_cs_awk_pipe_fini])
_ACAWK
_ACEOF
dnl See if CR is the EOL marker.  If not, remove any EOL-related
dnl ^M bytes and escape any remaining ones.  If so, just use mv.
dnl In case you're wondering how ^M bytes can make it into subs1.awk,
dnl [from Ralf Wildenhues] one way is if you have e.g.,
dnl AC_SUBST([variable_that_contains_cr], ["
dnl "])
dnl The original aim was that users should be able to substitute any
dnl characters they like (except for \0).  And the above is not so
dnl unlikely if the configure script itself happens to be converted
dnl to w32 text mode.
cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1
if sed "s/$ac_cr//" < /dev/null > /dev/null 2>&1; then
  sed "s/$ac_cr\$//; s/$ac_cr/$ac_cs_awk_cr/g"
else
  cat
fi < "$ac_tmp/subs1.awk" > "$ac_tmp/subs.awk" \
  || AC_MSG_ERROR([could not setup config files machinery])
_ACEOF

# VPATH may cause trouble with some makes, so we remove sole $(srcdir),
# ${srcdir} and @srcdir@ entries from VPATH if srcdir is ".", strip leading and
# trailing colons and then remove the whole line if VPATH becomes empty
# (actually we leave an empty line to preserve line numbers).
if test "x$srcdir" = x.; then
  ac_vpsub=['/^[	 ]*VPATH[	 ]*=[	 ]*/{
h
s///
s/^/:/
s/[	 ]*$/:/
s/:\$(srcdir):/:/g
s/:\${srcdir}:/:/g
s/:@srcdir@:/:/g
s/^:*//
s/:*$//
x
s/\(=[	 ]*\).*/\1/
G
s/\n//
s/^[^=]*=[	 ]*$//
}']
fi

cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1
fi # test -n "$CONFIG_FILES"

])# _AC_OUTPUT_FILES_PREPARE

# _AC_OUTPUT_FILE_ADJUST_DIR(VAR)
# -------------------------------
# Generate the sed snippet needed to output VAR relative to the
# top-level directory.
m4_define([_AC_OUTPUT_FILE_ADJUST_DIR],
[s&@$1@&$ac_$1&;t t[]AC_SUBST_TRACE([$1])])


# _AC_OUTPUT_FILE
# ---------------
# Do the variable substitutions to create the Makefiles or whatever.
#
# This macro is expanded inside a here document.  If the here document is
# closed, it has to be reopened with
# "cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1".
#
m4_define([_AC_OUTPUT_FILE],
[
  #
  # CONFIG_FILE
  #

AC_PROVIDE_IFELSE([AC_PROG_INSTALL],
[  case $INSTALL in
  [[\\/$]]* | ?:[[\\/]]* ) ac_INSTALL=$INSTALL ;;
  *) ac_INSTALL=$ac_top_build_prefix$INSTALL ;;
  esac
])dnl
AC_PROVIDE_IFELSE([AC_PROG_MKDIR_P],
[  ac_MKDIR_P=$MKDIR_P
  case $MKDIR_P in
  [[\\/$]]* | ?:[[\\/]]* ) ;;
  */*) ac_MKDIR_P=$ac_top_build_prefix$MKDIR_P ;;
  esac
])dnl
_ACEOF

m4_ifndef([AC_DATAROOTDIR_CHECKED],
[cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1
# If the template does not know about datarootdir, expand it.
# FIXME: This hack should be removed a few years after 2.60.
ac_datarootdir_hack=; ac_datarootdir_seen=
m4_define([_AC_datarootdir_vars],
	  [datadir, docdir, infodir, localedir, mandir])]dnl
[m4_define([_AC_datarootdir_subst], [  s&@$][1@&$$][1&g])]dnl
[ac_sed_dataroot='
/datarootdir/ {
  p
  q
}
m4_map_args_sep([/@], [@/p], [
], _AC_datarootdir_vars)'
case `eval "sed -n \"\$ac_sed_dataroot\" $ac_file_inputs"` in
*datarootdir*) ac_datarootdir_seen=yes;;
*@[]m4_join([@*|*@], _AC_datarootdir_vars)@*)
  AC_MSG_WARN([$ac_file_inputs seems to ignore the --datarootdir setting])
_ACEOF
cat >>$CONFIG_STATUS <<_ACEOF || ac_write_fail=1
  ac_datarootdir_hack='
m4_map_args_sep([_AC_datarootdir_subst(], [)], [
], _AC_datarootdir_vars)
  s&\\\${datarootdir}&$datarootdir&g' ;;
esac
_ACEOF
])dnl

# Neutralize VPATH when `$srcdir' = `.'.
# Shell code in configure.ac might set extrasub.
# FIXME: do we really want to maintain this feature?
cat >>$CONFIG_STATUS <<_ACEOF || ac_write_fail=1
ac_sed_extra="$ac_vpsub
$extrasub
_ACEOF
cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1
:t
[/@[a-zA-Z_][a-zA-Z_0-9]*@/!b]
dnl configure_input is a somewhat special, so we don't call AC_SUBST_TRACE.
dnl Note if you change the s||| delimiter here, don't forget to adjust
dnl ac_sed_conf_input accordingly.  Using & is a bad idea if & appears in
dnl the replacement string.
s|@configure_input@|$ac_sed_conf_input|;t t
dnl During the transition period, this is a special case:
s&@top_builddir@&$ac_top_builddir_sub&;t t[]AC_SUBST_TRACE([top_builddir])
dnl For this substitution see the witness macro _AC_HAVE_TOP_BUILD_PREFIX above.
s&@top_build_prefix@&$ac_top_build_prefix&;t t[]AC_SUBST_TRACE([top_build_prefix])
m4_map_args_sep([$0_ADJUST_DIR(], [)], [
], [srcdir], [abs_srcdir], [top_srcdir], [abs_top_srcdir],
   [builddir], [abs_builddir],
   [abs_top_builddir]AC_PROVIDE_IFELSE([AC_PROG_INSTALL],
     [, [INSTALL]])AC_PROVIDE_IFELSE([AC_PROG_MKDIR_P], [, [MKDIR_P]]))
m4_ifndef([AC_DATAROOTDIR_CHECKED], [$ac_datarootdir_hack
])dnl
"
eval sed \"\$ac_sed_extra\" "$ac_file_inputs" m4_defn([_AC_SUBST_CMDS]) \
  >$ac_tmp/out || AC_MSG_ERROR([could not create $ac_file])

m4_ifndef([AC_DATAROOTDIR_CHECKED],
[test -z "$ac_datarootdir_hack$ac_datarootdir_seen" &&
  { ac_out=`sed -n '/\${datarootdir}/p' "$ac_tmp/out"`; test -n "$ac_out"; } &&
  { ac_out=`sed -n '/^[[	 ]]*datarootdir[[	 ]]*:*=/p' \
      "$ac_tmp/out"`; test -z "$ac_out"; } &&
  AC_MSG_WARN([$ac_file contains a reference to the variable `datarootdir'
which seems to be undefined.  Please make sure it is defined])
])dnl

  rm -f "$ac_tmp/stdin"
  case $ac_file in
  -) cat "$ac_tmp/out" && rm -f "$ac_tmp/out";;
  *) rm -f "$ac_file" && mv "$ac_tmp/out" "$ac_file";;
  esac \
  || AC_MSG_ERROR([could not create $ac_file])
dnl This would break Makefile dependencies:
dnl  if diff "$ac_file" "$ac_tmp/out" >/dev/null 2>&1; then
dnl    echo "$ac_file is unchanged"
dnl  else
dnl     rm -f "$ac_file"; mv "$ac_tmp/out" "$ac_file"
dnl  fi
])# _AC_OUTPUT_FILE




## ----------------------- ##
## Configuration headers.  ##
## ----------------------- ##


# AC_CONFIG_HEADERS(HEADERS..., [COMMANDS], [INIT-CMDS])
# ------------------------------------------------------
# Specify that the HEADERS are to be created by instantiation of the
# AC_DEFINEs.
#
AC_DEFUN([AC_CONFIG_HEADERS], [_AC_CONFIG_FOOS([HEADERS], $@)])


# AC_CONFIG_HEADER(HEADER-TO-CREATE ...)
# --------------------------------------
# FIXME: Make it obsolete?
AC_DEFUN([AC_CONFIG_HEADER],
[AC_CONFIG_HEADERS([$1])])


# _AC_OUTPUT_HEADERS_PREPARE
# --------------------------
# Create the awk scripts needed for CONFIG_HEADERS.
# Support multiline #defines.
#
# This macro is expanded inside a here document.  If the here document is
# closed, it has to be reopened with
# "cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1".
#
m4_define([_AC_OUTPUT_HEADERS_PREPARE],
[# Set up the scripts for CONFIG_HEADERS section.
# No need to generate them if there are no CONFIG_HEADERS.
# This happens for instance with `./config.status Makefile'.
if test -n "$CONFIG_HEADERS"; then
dnl This `||' list is finished at the end of _AC_OUTPUT_HEADERS_PREPARE.
cat >"$ac_tmp/defines.awk" <<\_ACAWK ||
BEGIN {
_ACEOF

# Transform confdefs.h into an awk script `defines.awk', embedded as
# here-document in config.status, that substitutes the proper values into
# config.h.in to produce config.h.

# Create a delimiter string that does not exist in confdefs.h, to ease
# handling of long lines.
ac_delim='%!_!# '
for ac_last_try in false false :; do
  ac_tt=`sed -n "/$ac_delim/p" confdefs.h`
  if test -z "$ac_tt"; then
    break
  elif $ac_last_try; then
    AC_MSG_ERROR([could not make $CONFIG_HEADERS])
  else
    ac_delim="$ac_delim!$ac_delim _$ac_delim!! "
  fi
done

# For the awk script, D is an array of macro values keyed by name,
# likewise P contains macro parameters if any.  Preserve backslash
# newline sequences.
dnl
dnl Structure of the sed script that reads confdefs.h:
dnl rset:  main loop, searches for `#define' lines
dnl def:   deal with a `#define' line
dnl bsnl:  deal with a `#define' line that ends with backslash-newline
dnl cont:  handle a continuation line
dnl bsnlc: handle a continuation line that ends with backslash-newline
dnl
dnl Each sub part escapes the awk special characters and outputs a statement
dnl inserting the macro value into the array D, keyed by name.  If the macro
dnl uses parameters, they are added in the array P, keyed by name.
dnl
dnl Long values are split into several string literals with help of ac_delim.
dnl Assume nobody uses macro names of nearly 150 bytes length.
dnl
dnl The initial replace for `#define' lines inserts a leading space
dnl in order to ease later matching; otherwise, output lines may be
dnl repeatedly matched.
dnl
dnl m4-double-quote most of this for [, ], define, and substr:
[
ac_word_re=[_$as_cr_Letters][_$as_cr_alnum]*
sed -n '
s/.\{]_AC_AWK_LITERAL_LIMIT[\}/&'"$ac_delim"'/g
t rset
:rset
s/^[	 ]*#[	 ]*define[	 ][	 ]*/ /
t def
d
:def
s/\\$//
t bsnl
s/["\\]/\\&/g
s/^ \('"$ac_word_re"'\)\(([^()]*)\)[	 ]*\(.*\)/P["\1"]="\2"\
D["\1"]=" \3"/p
s/^ \('"$ac_word_re"'\)[	 ]*\(.*\)/D["\1"]=" \2"/p
d
:bsnl
s/["\\]/\\&/g
s/^ \('"$ac_word_re"'\)\(([^()]*)\)[	 ]*\(.*\)/P["\1"]="\2"\
D["\1"]=" \3\\\\\\n"\\/p
t cont
s/^ \('"$ac_word_re"'\)[	 ]*\(.*\)/D["\1"]=" \2\\\\\\n"\\/p
t cont
d
:cont
n
s/.\{]_AC_AWK_LITERAL_LIMIT[\}/&'"$ac_delim"'/g
t clear
:clear
s/\\$//
t bsnlc
s/["\\]/\\&/g; s/^/"/; s/$/"/p
d
:bsnlc
s/["\\]/\\&/g; s/^/"/; s/$/\\\\\\n"\\/p
b cont
' <confdefs.h | sed '
s/'"$ac_delim"'/"\\\
"/g' >>$CONFIG_STATUS || ac_write_fail=1

cat >>$CONFIG_STATUS <<_ACEOF || ac_write_fail=1
  for (key in D) D_is_set[key] = 1
  FS = ""
}
/^[\t ]*#[\t ]*(define|undef)[\t ]+$ac_word_re([\t (]|\$)/ {
  line = \$ 0
  split(line, arg, " ")
  if (arg[1] == "#") {
    defundef = arg[2]
    mac1 = arg[3]
  } else {
    defundef = substr(arg[1], 2)
    mac1 = arg[2]
  }
  split(mac1, mac2, "(") #)
  macro = mac2[1]
  prefix = substr(line, 1, index(line, defundef) - 1)
  if (D_is_set[macro]) {
    # Preserve the white space surrounding the "#".
    print prefix "define", macro P[macro] D[macro]
    next
  } else {
    # Replace #undef with comments.  This is necessary, for example,
    # in the case of _POSIX_SOURCE, which is predefined and required
    # on some systems where configure will not decide to define it.
    if (defundef == "undef") {
      print "/*", prefix defundef, macro, "*/"
      next
    }
  }
}
{ print }
]dnl End of double-quoted section
_ACAWK
_ACEOF
cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1
dnl finish `||' list indicating write error:
  AC_MSG_ERROR([could not setup config headers machinery])
fi # test -n "$CONFIG_HEADERS"

])# _AC_OUTPUT_HEADERS_PREPARE


# _AC_OUTPUT_HEADER
# -----------------
#
# Output the code which instantiates the `config.h' files from their
# `config.h.in'.
#
# This macro is expanded inside a here document.  If the here document is
# closed, it has to be reopened with
# "cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1".
#
m4_define([_AC_OUTPUT_HEADER],
[
  #
  # CONFIG_HEADER
  #
  if test x"$ac_file" != x-; then
    {
      AS_ECHO(["/* $configure_input  */"]) \
      && eval '$AWK -f "$ac_tmp/defines.awk"' "$ac_file_inputs"
    } >"$ac_tmp/config.h" \
      || AC_MSG_ERROR([could not create $ac_file])
    if diff "$ac_file" "$ac_tmp/config.h" >/dev/null 2>&1; then
      AC_MSG_NOTICE([$ac_file is unchanged])
    else
      rm -f "$ac_file"
      mv "$ac_tmp/config.h" "$ac_file" \
	|| AC_MSG_ERROR([could not create $ac_file])
    fi
  else
    AS_ECHO(["/* $configure_input  */"]) \
      && eval '$AWK -f "$ac_tmp/defines.awk"' "$ac_file_inputs" \
      || AC_MSG_ERROR([could not create -])
  fi
dnl If running for Automake, be ready to perform additional
dnl commands to set up the timestamp files.
m4_ifdef([_AC_AM_CONFIG_HEADER_HOOK],
	 [_AC_AM_CONFIG_HEADER_HOOK(["$ac_file"])
])dnl
])# _AC_OUTPUT_HEADER



## --------------------- ##
## Configuration links.  ##
## --------------------- ##


# AC_CONFIG_LINKS(DEST:SOURCE..., [COMMANDS], [INIT-CMDS])
# --------------------------------------------------------
# Specify that config.status should establish a (symbolic if possible)
# link from TOP_SRCDIR/SOURCE to TOP_SRCDIR/DEST.
# Reject DEST=., because it is makes it hard for ./config.status
# to guess the links to establish (`./config.status .').
#
AC_DEFUN([AC_CONFIG_LINKS], [_AC_CONFIG_FOOS([LINKS], $@)])


# AC_LINK_FILES(SOURCE..., DEST...)
# ---------------------------------
# Link each of the existing files SOURCE... to the corresponding
# link name in DEST...
#
# Unfortunately we can't provide a very good autoupdate service here,
# since in `AC_LINK_FILES($from, $to)' it is possible that `$from'
# and `$to' are actually lists.  It would then be completely wrong to
# replace it with `AC_CONFIG_LINKS($to:$from).  It is possible in the
# case of literal values though, but because I don't think there is any
# interest in creating config links with literal values, no special
# mechanism is implemented to handle them.
#
# _AC_LINK_FILES_CNT is used to be robust to multiple calls.
AU_DEFUN([AC_LINK_FILES],
[m4_if($#, 2, ,
       [m4_fatal([$0: incorrect number of arguments])])dnl
m4_define_default([_AC_LINK_FILES_CNT], 0)dnl
m4_define([_AC_LINK_FILES_CNT], m4_incr(_AC_LINK_FILES_CNT))dnl
ac_sources="$1"
ac_dests="$2"
while test -n "$ac_sources"; do
  set $ac_dests; ac_dest=$[1]; shift; ac_dests=$[*]
  set $ac_sources; ac_source=$[1]; shift; ac_sources=$[*]
  [ac_config_links_]_AC_LINK_FILES_CNT="$[ac_config_links_]_AC_LINK_FILES_CNT $ac_dest:$ac_source"
done
AC_CONFIG_LINKS($[ac_config_links_]_AC_LINK_FILES_CNT)dnl
],
[It is technically impossible to `autoupdate' cleanly from AC_LINK_FILES
to AC_CONFIG_LINKS.  `autoupdate' provides a functional but inelegant
update, you should probably tune the result yourself.])# AC_LINK_FILES


# _AC_OUTPUT_LINK
# ---------------
# This macro is expanded inside a here document.  If the here document is
# closed, it has to be reopened with
# "cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1".
m4_define([_AC_OUTPUT_LINK],
[
  #
  # CONFIG_LINK
  #

  if test "$ac_source" = "$ac_file" && test "$srcdir" = '.'; then
    :
  else
    # Prefer the file from the source tree if names are identical.
    if test "$ac_source" = "$ac_file" || test ! -r "$ac_source"; then
      ac_source=$srcdir/$ac_source
    fi

    AC_MSG_NOTICE([linking $ac_source to $ac_file])

    if test ! -r "$ac_source"; then
      AC_MSG_ERROR([$ac_source: file not found])
    fi
    rm -f "$ac_file"

    # Try a relative symlink, then a hard link, then a copy.
    case $ac_source in
    [[\\/$]]* | ?:[[\\/]]* ) ac_rel_source=$ac_source ;;
	*) ac_rel_source=$ac_top_build_prefix$ac_source ;;
    esac
    ln -s "$ac_rel_source" "$ac_file" 2>/dev/null ||
      ln "$ac_source" "$ac_file" 2>/dev/null ||
      cp -p "$ac_source" "$ac_file" ||
      AC_MSG_ERROR([cannot link or copy $ac_source to $ac_file])
  fi
])# _AC_OUTPUT_LINK



## ------------------------ ##
## Configuration commands.  ##
## ------------------------ ##


# AC_CONFIG_COMMANDS(NAME...,[COMMANDS], [INIT-CMDS])
# ---------------------------------------------------
#
# Specify additional commands to be run by config.status.  This
# commands must be associated with a NAME, which should be thought
# as the name of a file the COMMANDS create.
#
AC_DEFUN([AC_CONFIG_COMMANDS], [_AC_CONFIG_FOOS([COMMANDS], $@)])


# AC_OUTPUT_COMMANDS(EXTRA-CMDS, INIT-CMDS)
# -----------------------------------------
#
# Add additional commands for AC_OUTPUT to put into config.status.
#
# This macro is an obsolete version of AC_CONFIG_COMMANDS.  The only
# difficulty in mapping AC_OUTPUT_COMMANDS to AC_CONFIG_COMMANDS is
# to give a unique key.  The scheme we have chosen is `default-1',
# `default-2' etc. for each call.
#
# Unfortunately this scheme is fragile: bad things might happen
# if you update an included file and configure.ac: you might have
# clashes :(  On the other hand, I'd like to avoid weird keys (e.g.,
# depending upon __file__ or the pid).
AU_DEFUN([AC_OUTPUT_COMMANDS],
[m4_define_default([_AC_OUTPUT_COMMANDS_CNT], 0)dnl
m4_define([_AC_OUTPUT_COMMANDS_CNT], m4_incr(_AC_OUTPUT_COMMANDS_CNT))dnl
dnl Double quoted since that was the case in the original macro.
AC_CONFIG_COMMANDS([default-]_AC_OUTPUT_COMMANDS_CNT, [[$1]], [[$2]])dnl
])


# _AC_OUTPUT_COMMAND
# ------------------
# This macro is expanded inside a here document.  If the here document is
# closed, it has to be reopened with
# "cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1".
m4_define([_AC_OUTPUT_COMMAND],
[  AC_MSG_NOTICE([executing $ac_file commands])
])



## -------------------------------------- ##
## Pre- and post-config.status commands.  ##
## -------------------------------------- ##


# AC_CONFIG_COMMANDS_PRE(CMDS)
# ----------------------------
# Commands to run right before config.status is created. Accumulates.
AC_DEFUN([AC_CONFIG_COMMANDS_PRE],
[m4_append([AC_OUTPUT_COMMANDS_PRE], [$1
])])


# AC_OUTPUT_COMMANDS_PRE
# ----------------------
# A *variable* in which we append all the actions that must be
# performed before *creating* config.status.  For a start, clean
# up all the LIBOBJ mess.
m4_define([AC_OUTPUT_COMMANDS_PRE],
[_AC_LIBOBJS_NORMALIZE
])


# AC_CONFIG_COMMANDS_POST(CMDS)
# -----------------------------
# Commands to run after config.status was created.  Accumulates.
AC_DEFUN([AC_CONFIG_COMMANDS_POST],
[m4_append([AC_OUTPUT_COMMANDS_POST], [$1
])])

# Initialize.
m4_define([AC_OUTPUT_COMMANDS_POST])



## ----------------------- ##
## Configuration subdirs.  ##
## ----------------------- ##


# AC_CONFIG_SUBDIRS(DIR ...)
# --------------------------
# We define two variables:
# - _AC_LIST_SUBDIRS
#   A statically built list, should contain *all* the arguments of
#   AC_CONFIG_SUBDIRS.  The final value is assigned to ac_subdirs_all in
#   the `default' section, and used for --help=recursive.
#   It makes no sense for arguments which are sh variables.
# - subdirs
#   Shell variable built at runtime, so some of these dirs might not be
#   included, if for instance the user refused a part of the tree.
#   This is used in _AC_OUTPUT_SUBDIRS.
AC_DEFUN([AC_CONFIG_SUBDIRS],
[AC_REQUIRE([AC_CONFIG_AUX_DIR_DEFAULT])]dnl
[AC_REQUIRE([AC_DISABLE_OPTION_CHECKING])]dnl
[m4_map_args_w([$1], [_AC_CONFIG_UNIQUE([SUBDIRS],
  _AC_CONFIG_COMPUTE_DEST(], [))])]dnl
[m4_append([_AC_LIST_SUBDIRS], [$1], [
])]dnl
[AS_LITERAL_IF([$1], [],
	       [AC_DIAGNOSE([syntax], [$0: you should use literals])])]dnl
[AC_SUBST([subdirs], ["$subdirs m4_normalize([$1])"])])


# _AC_OUTPUT_SUBDIRS
# ------------------
# This is a subroutine of AC_OUTPUT, but it does not go into
# config.status, rather, it is called after running config.status.
m4_define([_AC_OUTPUT_SUBDIRS],
[
#
# CONFIG_SUBDIRS section.
#
if test "$no_recursion" != yes; then

  # Remove --cache-file, --srcdir, and --disable-option-checking arguments
  # so they do not pile up.
  ac_sub_configure_args=
  ac_prev=
  eval "set x $ac_configure_args"
  shift
  for ac_arg
  do
    if test -n "$ac_prev"; then
      ac_prev=
      continue
    fi
    case $ac_arg in
    -cache-file | --cache-file | --cache-fil | --cache-fi \
    | --cache-f | --cache- | --cache | --cach | --cac | --ca | --c)
      ac_prev=cache_file ;;
    -cache-file=* | --cache-file=* | --cache-fil=* | --cache-fi=* \
    | --cache-f=* | --cache-=* | --cache=* | --cach=* | --cac=* | --ca=* \
    | --c=*)
      ;;
    --config-cache | -C)
      ;;
    -srcdir | --srcdir | --srcdi | --srcd | --src | --sr)
      ac_prev=srcdir ;;
    -srcdir=* | --srcdir=* | --srcdi=* | --srcd=* | --src=* | --sr=*)
      ;;
    -prefix | --prefix | --prefi | --pref | --pre | --pr | --p)
      ac_prev=prefix ;;
    -prefix=* | --prefix=* | --prefi=* | --pref=* | --pre=* | --pr=* | --p=*)
      ;;
    --disable-option-checking)
      ;;
    *)
      case $ac_arg in
      *\'*) ac_arg=`AS_ECHO(["$ac_arg"]) | sed "s/'/'\\\\\\\\''/g"` ;;
      esac
      AS_VAR_APPEND([ac_sub_configure_args], [" '$ac_arg'"]) ;;
    esac
  done

  # Always prepend --prefix to ensure using the same prefix
  # in subdir configurations.
  ac_arg="--prefix=$prefix"
  case $ac_arg in
  *\'*) ac_arg=`AS_ECHO(["$ac_arg"]) | sed "s/'/'\\\\\\\\''/g"` ;;
  esac
  ac_sub_configure_args="'$ac_arg' $ac_sub_configure_args"

  # Pass --silent
  if test "$silent" = yes; then
    ac_sub_configure_args="--silent $ac_sub_configure_args"
  fi

  # Always prepend --disable-option-checking to silence warnings, since
  # different subdirs can have different --enable and --with options.
  ac_sub_configure_args="--disable-option-checking $ac_sub_configure_args"

  ac_popdir=`pwd`
  for ac_dir in : $subdirs; do test "x$ac_dir" = x: && continue

    # Do not complain, so a configure script can configure whichever
    # parts of a large source tree are present.
    test -d "$srcdir/$ac_dir" || continue

    ac_msg="=== configuring in $ac_dir (`pwd`/$ac_dir)"
    _AS_ECHO_LOG([$ac_msg])
    _AS_ECHO([$ac_msg])
    AS_MKDIR_P(["$ac_dir"])
    _AC_SRCDIRS(["$ac_dir"])

    cd "$ac_dir"

    # Check for guested configure; otherwise get Cygnus style configure.
    if test -f "$ac_srcdir/configure.gnu"; then
      ac_sub_configure=$ac_srcdir/configure.gnu
    elif test -f "$ac_srcdir/configure"; then
      ac_sub_configure=$ac_srcdir/configure
    elif test -f "$ac_srcdir/configure.in"; then
      # This should be Cygnus configure.
      ac_sub_configure=$ac_aux_dir/configure
    else
      AC_MSG_WARN([no configuration information is in $ac_dir])
      ac_sub_configure=
    fi

    # The recursion is here.
    if test -n "$ac_sub_configure"; then
      # Make the cache file name correct relative to the subdirectory.
      case $cache_file in
      [[\\/]]* | ?:[[\\/]]* ) ac_sub_cache_file=$cache_file ;;
      *) # Relative name.
	ac_sub_cache_file=$ac_top_build_prefix$cache_file ;;
      esac

      AC_MSG_NOTICE([running $SHELL $ac_sub_configure $ac_sub_configure_args --cache-file=$ac_sub_cache_file --srcdir=$ac_srcdir])
      # The eval makes quoting arguments work.
      eval "\$SHELL \"\$ac_sub_configure\" $ac_sub_configure_args \
	   --cache-file=\"\$ac_sub_cache_file\" --srcdir=\"\$ac_srcdir\"" ||
	AC_MSG_ERROR([$ac_sub_configure failed for $ac_dir])
    fi

    cd "$ac_popdir"
  done
fi
])# _AC_OUTPUT_SUBDIRS




## -------------------------- ##
## Outputting config.status.  ##
## -------------------------- ##


# AU::AC_OUTPUT([CONFIG_FILES...], [EXTRA-CMDS], [INIT-CMDS])
# -----------------------------------------------------------
#
# If there are arguments given to AC_OUTPUT, dispatch them to the
# proper modern macros.
AU_DEFUN([AC_OUTPUT],
[m4_ifvaln([$1],
	   [AC_CONFIG_FILES([$1])])dnl
m4_ifvaln([$2$3],
	  [AC_CONFIG_COMMANDS(default, [$2], [$3])])dnl
[AC_OUTPUT]])


# AC_OUTPUT([CONFIG_FILES...], [EXTRA-CMDS], [INIT-CMDS])
# -------------------------------------------------------
# The big finish.
# Produce config.status, config.h, and links; and configure subdirs.
#
m4_define([AC_OUTPUT],
[dnl Dispatch the extra arguments to their native macros.
m4_ifvaln([$1],
	  [AC_CONFIG_FILES([$1])])dnl
m4_ifvaln([$2$3],
	  [AC_CONFIG_COMMANDS(default, [$2], [$3])])dnl
m4_ifval([$1$2$3],
	 [AC_DIAGNOSE([obsolete],
		      [$0 should be used without arguments.
You should run autoupdate.])])dnl
AC_CACHE_SAVE

test "x$prefix" = xNONE && prefix=$ac_default_prefix
# Let make expand exec_prefix.
test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'

m4_ifdef([_AC_SEEN_CONFIG(HEADERS)], [DEFS=-DHAVE_CONFIG_H], [AC_OUTPUT_MAKE_DEFS()])

dnl Commands to run before creating config.status.
AC_OUTPUT_COMMANDS_PRE()dnl

: "${CONFIG_STATUS=./config.status}"
ac_write_fail=0
ac_clean_files_save=$ac_clean_files
ac_clean_files="$ac_clean_files $CONFIG_STATUS"
_AC_OUTPUT_CONFIG_STATUS()dnl
ac_clean_files=$ac_clean_files_save

test $ac_write_fail = 0 ||
  AC_MSG_ERROR([write failure creating $CONFIG_STATUS])

dnl Commands to run after config.status was created
AC_OUTPUT_COMMANDS_POST()dnl

# configure is writing to config.log, and then calls config.status.
# config.status does its own redirection, appending to config.log.
# Unfortunately, on DOS this fails, as config.log is still kept open
# by configure, so config.status won't be able to write to it; its
# output is simply discarded.  So we exec the FD to /dev/null,
# effectively closing config.log, so it can be properly (re)opened and
# appended to by config.status.  When coming back to configure, we
# need to make the FD available again.
if test "$no_create" != yes; then
  ac_cs_success=:
  ac_config_status_args=
  test "$silent" = yes &&
    ac_config_status_args="$ac_config_status_args --quiet"
  exec AS_MESSAGE_LOG_FD>/dev/null
  $SHELL $CONFIG_STATUS $ac_config_status_args || ac_cs_success=false
  exec AS_MESSAGE_LOG_FD>>config.log
  # Use ||, not &&, to avoid exiting from the if with $? = 1, which
  # would make configure fail if this is the last instruction.
  $ac_cs_success || AS_EXIT([1])
fi
dnl config.status should not do recursion.
AC_PROVIDE_IFELSE([AC_CONFIG_SUBDIRS], [_AC_OUTPUT_SUBDIRS()])dnl
if test -n "$ac_unrecognized_opts" && test "$enable_option_checking" != no; then
  AC_MSG_WARN([unrecognized options: $ac_unrecognized_opts])
fi
])# AC_OUTPUT


# _AC_OUTPUT_CONFIG_STATUS
# ------------------------
# Produce config.status.  Called by AC_OUTPUT.
# Pay special attention not to have too long here docs: some old
# shells die.  Unfortunately the limit is not known precisely...
m4_define([_AC_OUTPUT_CONFIG_STATUS],
[AC_MSG_NOTICE([creating $CONFIG_STATUS])
dnl AS_MESSAGE_LOG_FD is not available yet:
m4_pushdef([AS_MESSAGE_LOG_FD])]dnl
[AS_INIT_GENERATED([$CONFIG_STATUS],
[# Run this file to recreate the current configuration.
# Compiler output produced by configure, useful for debugging
# configure, is in config.log if it exists.

debug=false
ac_cs_recheck=false
ac_cs_silent=false
]) || ac_write_fail=1

cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1
[#] Save the log message, to keep $[0] and so on meaningful, and to
# report actual input values of CONFIG_FILES etc. instead of their
# values after options handling.
ac_log="
This file was extended by m4_ifset([AC_PACKAGE_NAME], [AC_PACKAGE_NAME ])dnl
$as_me[]m4_ifset([AC_PACKAGE_VERSION], [ AC_PACKAGE_VERSION]), which was
generated by m4_PACKAGE_STRING.  Invocation command line was

  CONFIG_FILES    = $CONFIG_FILES
  CONFIG_HEADERS  = $CONFIG_HEADERS
  CONFIG_LINKS    = $CONFIG_LINKS
  CONFIG_COMMANDS = $CONFIG_COMMANDS
  $ $[0] $[@]

on `(hostname || uname -n) 2>/dev/null | sed 1q`
"

_ACEOF

dnl remove any newlines from these variables.
m4_ifdef([_AC_SEEN_CONFIG(FILES)],
[case $ac_config_files in *"
"*) set x $ac_config_files; shift; ac_config_files=$[*];;
esac
])
m4_ifdef([_AC_SEEN_CONFIG(HEADERS)],
[case $ac_config_headers in *"
"*) set x $ac_config_headers; shift; ac_config_headers=$[*];;
esac
])

cat >>$CONFIG_STATUS <<_ACEOF || ac_write_fail=1
# Files that config.status was made for.
m4_ifdef([_AC_SEEN_CONFIG(FILES)],
[config_files="$ac_config_files"
])dnl
m4_ifdef([_AC_SEEN_CONFIG(HEADERS)],
[config_headers="$ac_config_headers"
])dnl
m4_ifdef([_AC_SEEN_CONFIG(LINKS)],
[config_links="$ac_config_links"
])dnl
m4_ifdef([_AC_SEEN_CONFIG(COMMANDS)],
[config_commands="$ac_config_commands"
])dnl

_ACEOF

cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1
ac_cs_usage="\
\`$as_me' instantiates files and other configuration actions
from templates according to the current configuration.  Unless the files
and actions are specified as TAGs, all are instantiated by default.

Usage: $[0] [[OPTION]]... [[TAG]]...

  -h, --help       print this help, then exit
  -V, --version    print version number and configuration settings, then exit
      --config     print configuration, then exit
  -q, --quiet, --silent
[]                   do not print progress messages
  -d, --debug      don't remove temporary files
      --recheck    update $as_me by reconfiguring in the same conditions
m4_ifdef([_AC_SEEN_CONFIG(FILES)],
  [AS_HELP_STRING([[    --file=FILE[:TEMPLATE]]],
    [instantiate the configuration file FILE], [                   ])
])dnl
m4_ifdef([_AC_SEEN_CONFIG(HEADERS)],
  [AS_HELP_STRING([[    --header=FILE[:TEMPLATE]]],
    [instantiate the configuration header FILE], [                   ])
])dnl

m4_ifdef([_AC_SEEN_CONFIG(FILES)],
[Configuration files:
$config_files

])dnl
m4_ifdef([_AC_SEEN_CONFIG(HEADERS)],
[Configuration headers:
$config_headers

])dnl
m4_ifdef([_AC_SEEN_CONFIG(LINKS)],
[Configuration links:
$config_links

])dnl
m4_ifdef([_AC_SEEN_CONFIG(COMMANDS)],
[Configuration commands:
$config_commands

])dnl
Report bugs to m4_ifset([AC_PACKAGE_BUGREPORT], [<AC_PACKAGE_BUGREPORT>],
  [the package provider]).dnl
m4_ifdef([AC_PACKAGE_NAME], [m4_ifset([AC_PACKAGE_URL], [
AC_PACKAGE_NAME home page: <AC_PACKAGE_URL>.])dnl
m4_if(m4_index(m4_defn([AC_PACKAGE_NAME]), [GNU ]), [0], [
General help using GNU software: <http://www.gnu.org/gethelp/>.])])"

_ACEOF
cat >>$CONFIG_STATUS <<_ACEOF || ac_write_fail=1
ac_cs_config="`AS_ECHO(["$ac_configure_args"]) | sed 's/^ //; s/[[\\""\`\$]]/\\\\&/g'`"
ac_cs_version="\\
m4_ifset([AC_PACKAGE_NAME], [AC_PACKAGE_NAME ])config.status[]dnl
m4_ifset([AC_PACKAGE_VERSION], [ AC_PACKAGE_VERSION])
configured by $[0], generated by m4_PACKAGE_STRING,
  with options \\"\$ac_cs_config\\"

Copyright (C) m4_PACKAGE_YEAR Free Software Foundation, Inc.
This config.status script is free software; the Free Software Foundation
gives unlimited permission to copy, distribute and modify it."

ac_pwd='$ac_pwd'
srcdir='$srcdir'
AC_PROVIDE_IFELSE([AC_PROG_INSTALL],
[INSTALL='$INSTALL'
])dnl
AC_PROVIDE_IFELSE([AC_PROG_MKDIR_P],
[MKDIR_P='$MKDIR_P'
])dnl
AC_PROVIDE_IFELSE([AC_PROG_AWK],
[AWK='$AWK'
])dnl
test -n "\$AWK" || AWK=awk
_ACEOF

cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1
# The default lists apply if the user does not specify any file.
ac_need_defaults=:
while test $[#] != 0
do
  case $[1] in
  --*=?*)
    ac_option=`expr "X$[1]" : 'X\([[^=]]*\)='`
    ac_optarg=`expr "X$[1]" : 'X[[^=]]*=\(.*\)'`
    ac_shift=:
    ;;
  --*=)
    ac_option=`expr "X$[1]" : 'X\([[^=]]*\)='`
    ac_optarg=
    ac_shift=:
    ;;
  *)
    ac_option=$[1]
    ac_optarg=$[2]
    ac_shift=shift
    ;;
  esac

  case $ac_option in
  # Handling of the options.
  -recheck | --recheck | --rechec | --reche | --rech | --rec | --re | --r)
    ac_cs_recheck=: ;;
  --version | --versio | --versi | --vers | --ver | --ve | --v | -V )
    AS_ECHO(["$ac_cs_version"]); exit ;;
  --config | --confi | --conf | --con | --co | --c )
    AS_ECHO(["$ac_cs_config"]); exit ;;
  --debug | --debu | --deb | --de | --d | -d )
    debug=: ;;
m4_ifdef([_AC_SEEN_CONFIG(FILES)], [dnl
  --file | --fil | --fi | --f )
    $ac_shift
    case $ac_optarg in
    *\'*) ac_optarg=`AS_ECHO(["$ac_optarg"]) | sed "s/'/'\\\\\\\\''/g"` ;;
    '') AC_MSG_ERROR([missing file argument]) ;;
    esac
    AS_VAR_APPEND([CONFIG_FILES], [" '$ac_optarg'"])
    ac_need_defaults=false;;
])dnl
m4_ifdef([_AC_SEEN_CONFIG(HEADERS)], [dnl
  --header | --heade | --head | --hea )
    $ac_shift
    case $ac_optarg in
    *\'*) ac_optarg=`AS_ECHO(["$ac_optarg"]) | sed "s/'/'\\\\\\\\''/g"` ;;
    esac
    AS_VAR_APPEND([CONFIG_HEADERS], [" '$ac_optarg'"])
    ac_need_defaults=false;;
  --he | --h)
    # Conflict between --help and --header
    AC_MSG_ERROR([ambiguous option: `$[1]'
Try `$[0] --help' for more information.]);;
], [  --he | --h |])dnl
  --help | --hel | -h )
    AS_ECHO(["$ac_cs_usage"]); exit ;;
  -q | -quiet | --quiet | --quie | --qui | --qu | --q \
  | -silent | --silent | --silen | --sile | --sil | --si | --s)
    ac_cs_silent=: ;;

  # This is an error.
  -*) AC_MSG_ERROR([unrecognized option: `$[1]'
Try `$[0] --help' for more information.]) ;;

  *) AS_VAR_APPEND([ac_config_targets], [" $[1]"])
     ac_need_defaults=false ;;

  esac
  shift
done

ac_configure_extra_args=

if $ac_cs_silent; then
  exec AS_MESSAGE_FD>/dev/null
  ac_configure_extra_args="$ac_configure_extra_args --silent"
fi

_ACEOF
cat >>$CONFIG_STATUS <<_ACEOF || ac_write_fail=1
dnl Check this before opening the log, to avoid a bug on MinGW,
dnl which prohibits the recursive instance from truncating an open log.
if \$ac_cs_recheck; then
  set X '$SHELL' '$[0]' $ac_configure_args \$ac_configure_extra_args --no-create --no-recursion
  shift
  \AS_ECHO(["running CONFIG_SHELL=$SHELL \$[*]"]) >&AS_MESSAGE_FD
  CONFIG_SHELL='$SHELL'
  export CONFIG_SHELL
  exec "\$[@]"
fi

_ACEOF
cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1
dnl Open the log:
m4_popdef([AS_MESSAGE_LOG_FD])dnl
exec AS_MESSAGE_LOG_FD>>config.log
{
  echo
  AS_BOX([Running $as_me.])
  AS_ECHO(["$ac_log"])
} >&AS_MESSAGE_LOG_FD

_ACEOF
cat >>$CONFIG_STATUS <<_ACEOF || ac_write_fail=1
m4_ifdef([_AC_OUTPUT_COMMANDS_INIT],
[#
# INIT-COMMANDS
#
_AC_OUTPUT_COMMANDS_INIT
])dnl
_ACEOF

cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1

# Handling of arguments.
for ac_config_target in $ac_config_targets
do
  case $ac_config_target in
m4_ifdef([_AC_LIST_TAGS], [_AC_LIST_TAGS])
  *) AC_MSG_ERROR([invalid argument: `$ac_config_target']);;
  esac
done

m4_ifdef([_AC_SEEN_CONFIG(ANY)], [_AC_OUTPUT_MAIN_LOOP])[]dnl

AS_EXIT(0)
_ACEOF
])# _AC_OUTPUT_CONFIG_STATUS

# _AC_OUTPUT_MAIN_LOOP
# --------------------
# The main loop in $CONFIG_STATUS.
#
# This macro is expanded inside a here document.  If the here document is
# closed, it has to be reopened with
# "cat >>$CONFIG_STATUS <<\_ACEOF || ac_write_fail=1".
#
AC_DEFUN([_AC_OUTPUT_MAIN_LOOP],
[
# If the user did not use the arguments to specify the items to instantiate,
# then the envvar interface is used.  Set only those that are not.
# We use the long form for the default assignment because of an extremely
# bizarre bug on SunOS 4.1.3.
if $ac_need_defaults; then
m4_ifdef([_AC_SEEN_CONFIG(FILES)],
[  test "${CONFIG_FILES+set}" = set || CONFIG_FILES=$config_files
])dnl
m4_ifdef([_AC_SEEN_CONFIG(HEADERS)],
[  test "${CONFIG_HEADERS+set}" = set || CONFIG_HEADERS=$config_headers
])dnl
m4_ifdef([_AC_SEEN_CONFIG(LINKS)],
[  test "${CONFIG_LINKS+set}" = set || CONFIG_LINKS=$config_links
])dnl
m4_ifdef([_AC_SEEN_CONFIG(COMMANDS)],
[  test "${CONFIG_COMMANDS+set}" = set || CONFIG_COMMANDS=$config_commands
])dnl
fi

# Have a temporary directory for convenience.  Make it in the build tree
# simply because there is no reason against having it here, and in addition,
# creating and moving files from /tmp can sometimes cause problems.
# Hook for its removal unless debugging.
# Note that there is a small window in which the directory will not be cleaned:
# after its creation but before its name has been assigned to `$tmp'.
dnl For historical reasons, AS_TMPDIR must continue to place the results
dnl in $tmp; but we swap to the namespace-clean $ac_tmp to avoid issues
dnl with any CONFIG_COMMANDS playing with the common variable name $tmp.
$debug ||
{
  tmp= ac_tmp=
  trap 'exit_status=$?
  : "${ac_tmp:=$tmp}"
  { test ! -d "$ac_tmp" || rm -fr "$ac_tmp"; } && exit $exit_status
' 0
  trap 'AS_EXIT([1])' 1 2 13 15
}
dnl The comment above AS_TMPDIR says at most 4 chars are allowed.
AS_TMPDIR([conf], [.])
ac_tmp=$tmp

m4_ifdef([_AC_SEEN_CONFIG(FILES)], [_AC_OUTPUT_FILES_PREPARE])[]dnl
m4_ifdef([_AC_SEEN_CONFIG(HEADERS)], [_AC_OUTPUT_HEADERS_PREPARE])[]dnl

eval set X "dnl
  m4_ifdef([_AC_SEEN_CONFIG(FILES)],    [:F $CONFIG_FILES])[]dnl
  m4_ifdef([_AC_SEEN_CONFIG(HEADERS)],  [:H $CONFIG_HEADERS])[]dnl
  m4_ifdef([_AC_SEEN_CONFIG(LINKS)],    [:L $CONFIG_LINKS])[]dnl
  m4_ifdef([_AC_SEEN_CONFIG(COMMANDS)], [:C $CONFIG_COMMANDS])[]dnl
"
shift
for ac_tag
do
  case $ac_tag in
  :[[FHLC]]) ac_mode=$ac_tag; continue;;
  esac
  case $ac_mode$ac_tag in
  :[[FHL]]*:*);;
  :L* | :C*:*) AC_MSG_ERROR([invalid tag `$ac_tag']);;
  :[[FH]]-) ac_tag=-:-;;
  :[[FH]]*) ac_tag=$ac_tag:$ac_tag.in;;
  esac
  ac_save_IFS=$IFS
  IFS=:
  set x $ac_tag
  IFS=$ac_save_IFS
  shift
  ac_file=$[1]
  shift

  case $ac_mode in
  :L) ac_source=$[1];;
  :[[FH]])
    ac_file_inputs=
    for ac_f
    do
      case $ac_f in
      -) ac_f="$ac_tmp/stdin";;
      *) # Look for the file first in the build tree, then in the source tree
	 # (if the path is not absolute).  The absolute path cannot be DOS-style,
	 # because $ac_f cannot contain `:'.
	 test -f "$ac_f" ||
	   case $ac_f in
	   [[\\/$]]*) false;;
	   *) test -f "$srcdir/$ac_f" && ac_f="$srcdir/$ac_f";;
	   esac ||
	   AC_MSG_ERROR([cannot find input file: `$ac_f'], [1]);;
      esac
      case $ac_f in *\'*) ac_f=`AS_ECHO(["$ac_f"]) | sed "s/'/'\\\\\\\\''/g"`;; esac
      AS_VAR_APPEND([ac_file_inputs], [" '$ac_f'"])
    done

    # Let's still pretend it is `configure' which instantiates (i.e., don't
    # use $as_me), people would be surprised to read:
    #    /* config.h.  Generated by config.status.  */
    configure_input='Generated from '`
	  AS_ECHO(["$[*]"]) | sed ['s|^[^:]*/||;s|:[^:]*/|, |g']
	`' by configure.'
    if test x"$ac_file" != x-; then
      configure_input="$ac_file.  $configure_input"
      AC_MSG_NOTICE([creating $ac_file])
    fi
    # Neutralize special characters interpreted by sed in replacement strings.
    case $configure_input in #(
    *\&* | *\|* | *\\* )
       ac_sed_conf_input=`AS_ECHO(["$configure_input"]) |
       sed 's/[[\\\\&|]]/\\\\&/g'`;; #(
    *) ac_sed_conf_input=$configure_input;;
    esac

    case $ac_tag in
    *:-:* | *:-) cat >"$ac_tmp/stdin" \
      || AC_MSG_ERROR([could not create $ac_file]) ;;
    esac
    ;;
  esac

  ac_dir=`AS_DIRNAME(["$ac_file"])`
  AS_MKDIR_P(["$ac_dir"])
  _AC_SRCDIRS(["$ac_dir"])

  case $ac_mode in
  m4_ifdef([_AC_SEEN_CONFIG(FILES)],    [:F)_AC_OUTPUT_FILE ;;])
  m4_ifdef([_AC_SEEN_CONFIG(HEADERS)],  [:H)_AC_OUTPUT_HEADER ;;])
  m4_ifdef([_AC_SEEN_CONFIG(LINKS)],    [:L)_AC_OUTPUT_LINK ;;])
  m4_ifdef([_AC_SEEN_CONFIG(COMMANDS)], [:C)_AC_OUTPUT_COMMAND ;;])
  esac

dnl Some shells don't like empty case/esac
m4_ifdef([_AC_LIST_TAG_COMMANDS], [
  case $ac_file$ac_mode in
_AC_LIST_TAG_COMMANDS
  esac
])dnl
done # for ac_tag

])# _AC_OUTPUT_MAIN_LOOP


# AC_OUTPUT_MAKE_DEFS
# -------------------
# Set the DEFS variable to the -D options determined earlier.
# This is a subroutine of AC_OUTPUT.
# It is called inside configure, outside of config.status.
m4_define([AC_OUTPUT_MAKE_DEFS],
[[# Transform confdefs.h into DEFS.
# Protect against shell expansion while executing Makefile rules.
# Protect against Makefile macro expansion.
#
# If the first sed substitution is executed (which looks for macros that
# take arguments), then branch to the quote section.  Otherwise,
# look for a macro that doesn't take arguments.
ac_script='
:mline
/\\$/{
 N
 s,\\\n,,
 b mline
}
t clear
:clear
s/^[	 ]*#[	 ]*define[	 ][	 ]*\([^	 (][^	 (]*([^)]*)\)[	 ]*\(.*\)/-D\1=\2/g
t quote
s/^[	 ]*#[	 ]*define[	 ][	 ]*\([^	 ][^	 ]*\)[	 ]*\(.*\)/-D\1=\2/g
t quote
b any
:quote
s/[	 `~#$^&*(){}\\|;'\''"<>?]/\\&/g
s/\[/\\&/g
s/\]/\\&/g
s/\$/$$/g
H
:any
${
	g
	s/^\n//
	s/\n/ /g
	p
}
'
DEFS=`sed -n "$ac_script" confdefs.h`
]])# AC_OUTPUT_MAKE_DEFS
