/*
 * these two functions are copied from glibc.
 */

#ifndef HAVE_WCSCASECMP
#include <wctype.h>

#include "MACUtils.h"

int mac_wcscasecmp(const wchar_t *s1, const wchar_t *s2)
{
  wint_t c1, c2;

  if (s1 == s2)
	return 0;

  do
  {
    c1 = towlower(*s1++);
    c2 = towlower(*s2++);
    if (c1 == L'\0')
      break;
  }
  while (c1 == c2);

  return c1 - c2;
}

int mac_wcsncasecmp(const wchar_t *s1, const wchar_t *s2, size_t n)
{
  wint_t c1, c2;

  if (s1 == s2 || n == 0)
    return 0;

  do
  {
    c1 = towlower(*s1++);
    c2 = towlower(*s2++);
    if (c1 == L'\0' || c1 != c2)
      return c1 - c2;
  } while (--n > 0);

  return c1 - c2;
}

#endif
