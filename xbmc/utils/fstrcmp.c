/*
 * Functions to make fuzzy comparisons between strings.
 *
 * Derived from PHP 5 similar_text() function
 *
 * The basic algorithm is described in:
 * Oliver [1993] and the complexity is O(N**3) with N == length of longest string

   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2010 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Rasmus Lerdorf <rasmus@php.net>                             |
   |          Stig Sther Bakken <ssb@php.net>                             |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>

static int similar_text(const char *str1, const char *str2, int len1, int len2)
{
  int sum;
  int pos1 = 0, pos2 = 0;
  int max = 0;

  char *p, *q;
  char *end1 = (char *)str1 + len1;
  char *end2 = (char *)str2 + len2;
  int l;

  for (p = (char *)str1; p < end1; p++)
  {
    for (q = (char *)str2; q < end2; q++)
    {
      for (l = 0; (p + l < end1) && (q + l < end2) && (p[l] == q[l]); l++)
        ;
      if (l > max)
      {
        max = l;
        pos1 = p - str1;
        pos2 = q - str2;
      }
    }
  }
  if ((sum = max))
  {
    if (pos1 && pos2)
      sum += similar_text(str1, str2, pos1, pos2);

    if ((pos1 + max < len1) && (pos2 + max < len2))
      sum += similar_text(str1 + pos1 + max, str2 + pos2 + max,
                          len1 - pos1 - max, len2 - pos2 - max);
  }

  return sum;
}

/* NAME
 fstrcmp - fuzzy string compare

   SYNOPSIS
 double fstrcmp(const char *, const char *, double);

   DESCRIPTION
 The fstrcmp function may be used to compare two string for
 similarity.  It is very useful in reducing "cascade" or
 "secondary" errors in compilers or other situations where
 symbol tables occur.

   RETURNS
 double; 0 if the strings are entirly dissimilar, 1 if the
 strings are identical, and a number in between if they are
 similar.  */

double
fstrcmp (const char *string1, const char *string2, double minimum)
{
  int len1, len2, score;

  len1 = (int)strlen(string1);
  len2 = (int)strlen(string2);

  /* short-circuit obvious comparisons */
  if (len1 == 0 && len2 == 0)
    return 1.0;
  if (len1 == 0 || len2 == 0)
    return 0.0;

  score = similar_text(string1, string2, len1, len2);
  /* The result is
  ((number of chars in common) / (average length of the strings)).
     This is admittedly biased towards finding that the strings are
     similar, however it does produce meaningful results.  */
  return ((double)score * 2.0 / (len1 + len2));
}

#ifdef __cplusplus
} // extern "C"
#endif
