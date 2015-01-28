/* basename.c -- return the last element in a file name

   Copyright (C) 1990, 1998, 1999, 2000, 2001, 2003, 2004, 2005, 2006 Free
   Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

#include "dirname.h"

#include <string.h>
#include "xalloc.h"
#include "xstrndup.h"

/* Return the address of the last file name component of NAME.  If
   NAME has no relative file name components because it is a file
   system root, return the empty string.  */

char *
last_component (char const *name)
{
  char const *base = name + FILE_SYSTEM_PREFIX_LEN (name);
  char const *p;
  bool saw_slash = false;

  while (ISSLASH (*base))
    base++;

  for (p = base; *p; p++)
    {
      if (ISSLASH (*p))
	saw_slash = true;
      else if (saw_slash)
	{
	  base = p;
	  saw_slash = false;
	}
    }

  return (char *) base;
}


/* In general, we can't use the builtin `basename' function if available,
   since it has different meanings in different environments.
   In some environments the builtin `basename' modifies its argument.

   Return the last file name component of NAME, allocated with
   xmalloc.  On systems with drive letters, a leading "./"
   distinguishes relative names that would otherwise look like a drive
   letter.  Unlike POSIX basename(), NAME cannot be NULL,
   base_name("") returns "", and the first trailing slash is not
   stripped.

   If lstat (NAME) would succeed, then { chdir (dir_name (NAME));
   lstat (base_name (NAME)); } will access the same file.  Likewise,
   if the sequence { chdir (dir_name (NAME));
   rename (base_name (NAME), "foo"); } succeeds, you have renamed NAME
   to "foo" in the same directory NAME was in.  */

char *
base_name (char const *name)
{
  char const *base = last_component (name);
  size_t length;

  /* If there is no last component, then name is a file system root or the
     empty string.  */
  if (! *base)
    return xstrndup (name, base_len (name));

  /* Collapse a sequence of trailing slashes into one.  */
  length = base_len (base);
  if (ISSLASH (base[length]))
    length++;

  /* On systems with drive letters, `a/b:c' must return `./b:c' rather
     than `b:c' to avoid confusion with a drive letter.  On systems
     with pure POSIX semantics, this is not an issue.  */
  if (FILE_SYSTEM_PREFIX_LEN (base))
    {
      char *p = xmalloc (length + 3);
      p[0] = '.';
      p[1] = '/';
      memcpy (p + 2, base, length);
      p[length + 2] = '\0';
      return p;
    }

  /* Finally, copy the basename.  */
  return xstrndup (base, length);
}

/* Return the length of the basename NAME.  Typically NAME is the
   value returned by base_name or last_component.  Act like strlen
   (NAME), except omit all trailing slashes.  */

size_t
base_len (char const *name)
{
  size_t len;
  size_t prefix_len = FILE_SYSTEM_PREFIX_LEN (name);

  for (len = strlen (name);  1 < len && ISSLASH (name[len - 1]);  len--)
    continue;

  if (DOUBLE_SLASH_IS_DISTINCT_ROOT && len == 1
      && ISSLASH (name[0]) && ISSLASH (name[1]) && ! name[2])
    return 2;

  if (FILE_SYSTEM_DRIVE_PREFIX_CAN_BE_RELATIVE && prefix_len
      && len == prefix_len && ISSLASH (name[prefix_len]))
    return prefix_len + 1;

  return len;
}
