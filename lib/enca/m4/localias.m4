## Locale alias location. This file is in public domain.
## Defines:
## HAVE_LOCALE_ALIAS when locale.alias is found
## LOCALE_ALIAS_PATH to path to locale.alias (has sense iff HAVE_LOCALE_ALIAS)
AC_DEFUN([ye_PATH_LOCALE_ALIAS],
[dnl Check for locale.alias
locale_alias_ok=no
if test "$ac_cv_func_setlocale" = yes; then
  AC_CACHE_CHECK([for locale.alias],
    yeti_cv_file_locale_alias,
    for yeti_ac_tmp in /usr/share/locale /usr/local/share/locale /etc /usr/lib/X11/locale /usr/X11/lib/locale; do
      if test -f "$yeti_ac_tmp/locale.alias"; then
        yeti_cv_file_locale_alias="$yeti_ac_tmp/locale.alias"
        break
      fi
      if test -f "$yeti_ac_tmp/locale.aliases"; then
        yeti_cv_file_locale_alias="$yeti_ac_tmp/locale.aliases"
        break
      fi
    done)
  if test -n "$yeti_cv_file_locale_alias"; then
    locale_alias_ok=yes
    AC_DEFINE(HAVE_LOCALE_ALIAS,1,[Define if you have locale.alias file.])
    AC_DEFINE_UNQUOTED(LOCALE_ALIAS_PATH,"$yeti_cv_file_locale_alias",[Define to the path to locale.alias file.])
  fi
fi])
