#serial 10   -*- autoconf -*-

# Define some macros required for proper operation of code in lib/*.c
# on MSDOS/Windows systems.

# Copyright (C) 2000, 2001, 2004, 2005, 2006 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# From Jim Meyering.

AC_DEFUN([gl_AC_DOS],
  [
    AC_CACHE_CHECK([whether system is Windows or MSDOS], [ac_cv_win_or_dos],
      [
	AC_TRY_COMPILE([],
	[#if !defined _WIN32 && !defined __WIN32__ && !defined __MSDOS__ && !defined __CYGWIN__
neither MSDOS nor Windows
#endif],
	[ac_cv_win_or_dos=yes],
	[ac_cv_win_or_dos=no])
      ])

    if test x"$ac_cv_win_or_dos" = xyes; then
      ac_fs_accepts_drive_letter_prefix=1
      ac_fs_backslash_is_file_name_separator=1
      AC_CACHE_CHECK([whether drive letter can start relative path],
		     [ac_cv_drive_letter_can_be_relative],
	[
	  AC_TRY_COMPILE([],
	  [#if defined __CYGWIN__
drive letters are always absolute
#endif],
	  [ac_cv_drive_letter_can_be_relative=yes],
	  [ac_cv_drive_letter_can_be_relative=no])
	])
      if test x"$ac_cv_drive_letter_can_be_relative" = xyes; then
	ac_fs_drive_letter_can_be_relative=1
      else
	ac_fs_drive_letter_can_be_relative=0
      fi
    else
      ac_fs_accepts_drive_letter_prefix=0
      ac_fs_backslash_is_file_name_separator=0
      ac_fs_drive_letter_can_be_relative=0
    fi

    AC_DEFINE_UNQUOTED([FILE_SYSTEM_ACCEPTS_DRIVE_LETTER_PREFIX],
      $ac_fs_accepts_drive_letter_prefix,
      [Define on systems for which file names may have a so-called
       `drive letter' prefix, define this to compute the length of that
       prefix, including the colon.])

    AH_VERBATIM(ISSLASH,
    [#if FILE_SYSTEM_BACKSLASH_IS_FILE_NAME_SEPARATOR
# define ISSLASH(C) ((C) == '/' || (C) == '\\')
#else
# define ISSLASH(C) ((C) == '/')
#endif])

    AC_DEFINE_UNQUOTED([FILE_SYSTEM_BACKSLASH_IS_FILE_NAME_SEPARATOR],
      $ac_fs_backslash_is_file_name_separator,
      [Define if the backslash character may also serve as a file name
       component separator.])

    AC_DEFINE_UNQUOTED([FILE_SYSTEM_DRIVE_PREFIX_CAN_BE_RELATIVE],
      $ac_fs_drive_letter_can_be_relative,
      [Define if a drive letter prefix denotes a relative path if it is
       not followed by a file name component separator.])
  ])
