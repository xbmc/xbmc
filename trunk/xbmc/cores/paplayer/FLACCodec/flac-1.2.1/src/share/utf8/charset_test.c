/*
 * Copyright (C) 2001 Edmund Grimley Evans <edmundo@rano.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include "charset.h"

void test_any(struct charset *charset)
{
  int wc;
  char s[2];

  assert(charset);

  /* Decoder */

  assert(charset_mbtowc(charset, 0, 0, 0) == 0);
  assert(charset_mbtowc(charset, 0, 0, 1) == 0);
  assert(charset_mbtowc(charset, 0, (char *)(-1), 0) == 0);

  assert(charset_mbtowc(charset, 0, "a", 0) == 0);
  assert(charset_mbtowc(charset, 0, "", 1) == 0);
  assert(charset_mbtowc(charset, 0, "b", 1) == 1);
  assert(charset_mbtowc(charset, 0, "", 2) == 0);
  assert(charset_mbtowc(charset, 0, "c", 2) == 1);

  wc = 'x';
  assert(charset_mbtowc(charset, &wc, "a", 0) == 0 && wc == 'x');
  assert(charset_mbtowc(charset, &wc, "", 1) == 0 && wc == 0);
  assert(charset_mbtowc(charset, &wc, "b", 1) == 1 && wc == 'b');
  assert(charset_mbtowc(charset, &wc, "", 2) == 0 && wc == 0);
  assert(charset_mbtowc(charset, &wc, "c", 2) == 1 && wc == 'c');

  /* Encoder */

  assert(charset_wctomb(charset, 0, 0) == 0);

  s[0] = s[1] = '.';
  assert(charset_wctomb(charset, s, 0) == 1 &&
	 s[0] == '\0' && s[1] == '.');
  assert(charset_wctomb(charset, s, 'x') == 1 &&
	 s[0] == 'x' && s[1] == '.');
}

void test_utf8()
{
  struct charset *charset;
  int wc;
  char s[8];

  charset = charset_find("UTF-8");
  test_any(charset);

  /* Decoder */
  wc = 0;
  assert(charset_mbtowc(charset, &wc, "\177", 1) == 1 && wc == 127);
  assert(charset_mbtowc(charset, &wc, "\200", 2) == -1);
  assert(charset_mbtowc(charset, &wc, "\301\277", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\302\200", 1) == -1);
  assert(charset_mbtowc(charset, &wc, "\302\200", 2) == 2 && wc == 128);
  assert(charset_mbtowc(charset, &wc, "\302\200", 3) == 2 && wc == 128);
  assert(charset_mbtowc(charset, &wc, "\340\237\200", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\340\240\200", 9) == 3 &&
	 wc == 1 << 11);
  assert(charset_mbtowc(charset, &wc, "\360\217\277\277", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\360\220\200\200", 9) == 4 &&
	 wc == 1 << 16);
  assert(charset_mbtowc(charset, &wc, "\370\207\277\277\277", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\370\210\200\200\200", 9) == 5 &&
	 wc == 1 << 21);
  assert(charset_mbtowc(charset, &wc, "\374\203\277\277\277\277", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\374\204\200\200\200\200", 9) == 6 &&
	 wc == 1 << 26);
  assert(charset_mbtowc(charset, &wc, "\375\277\277\277\277\277", 9) == 6 &&
	 wc == 0x7fffffff);

  assert(charset_mbtowc(charset, &wc, "\302\000", 2) == -1);
  assert(charset_mbtowc(charset, &wc, "\302\300", 2) == -1);
  assert(charset_mbtowc(charset, &wc, "\340\040\200", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\340\340\200", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\340\240\000", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\340\240\300", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\360\020\200\200", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\360\320\200\200", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\360\220\000\200", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\360\220\300\200", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\360\220\200\000", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\360\220\200\300", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\375\077\277\277\277\277", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\375\377\277\277\277\277", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\375\277\077\277\277\277", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\375\277\377\277\277\277", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\375\277\277\277\077\277", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\375\277\277\277\377\277", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\375\277\277\277\277\077", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\375\277\277\277\277\377", 9) == -1);

  assert(charset_mbtowc(charset, &wc, "\376\277\277\277\277\277", 9) == -1);
  assert(charset_mbtowc(charset, &wc, "\377\277\277\277\277\277", 9) == -1);

  /* Encoder */
  strcpy(s, ".......");
  assert(charset_wctomb(charset, s, 1 << 31) == -1 &&
	 !strcmp(s, "......."));
  assert(charset_wctomb(charset, s, 127) == 1 &&
	 !strcmp(s, "\177......"));
  assert(charset_wctomb(charset, s, 128) == 2 &&
	 !strcmp(s, "\302\200....."));
  assert(charset_wctomb(charset, s, 0x7ff) == 2 &&
	 !strcmp(s, "\337\277....."));
  assert(charset_wctomb(charset, s, 0x800) == 3 &&
	 !strcmp(s, "\340\240\200...."));
  assert(charset_wctomb(charset, s, 0xffff) == 3 &&
	 !strcmp(s, "\357\277\277...."));
  assert(charset_wctomb(charset, s, 0x10000) == 4 &&
	 !strcmp(s, "\360\220\200\200..."));
  assert(charset_wctomb(charset, s, 0x1fffff) == 4 &&
	 !strcmp(s, "\367\277\277\277..."));
  assert(charset_wctomb(charset, s, 0x200000) == 5 &&
	 !strcmp(s, "\370\210\200\200\200.."));
  assert(charset_wctomb(charset, s, 0x3ffffff) == 5 &&
	 !strcmp(s, "\373\277\277\277\277.."));
  assert(charset_wctomb(charset, s, 0x4000000) == 6 &&
	 !strcmp(s, "\374\204\200\200\200\200."));
  assert(charset_wctomb(charset, s, 0x7fffffff) == 6 &&
	 !strcmp(s, "\375\277\277\277\277\277."));
}

void test_ascii()
{
  struct charset *charset;
  int wc;
  char s[3];

  charset = charset_find("us-ascii");
  test_any(charset);

  /* Decoder */
  wc = 0;
  assert(charset_mbtowc(charset, &wc, "\177", 2) == 1 && wc == 127);
  assert(charset_mbtowc(charset, &wc, "\200", 2) == -1);

  /* Encoder */
  strcpy(s, "..");
  assert(charset_wctomb(charset, s, 256) == -1 && !strcmp(s, ".."));
  assert(charset_wctomb(charset, s, 255) == -1);
  assert(charset_wctomb(charset, s, 128) == -1);
  assert(charset_wctomb(charset, s, 127) == 1 && !strcmp(s, "\177."));
}

void test_iso1()
{
  struct charset *charset;
  int wc;
  char s[3];

  charset = charset_find("iso-8859-1");
  test_any(charset);

  /* Decoder */
  wc = 0;
  assert(charset_mbtowc(charset, &wc, "\302\200", 9) == 1 && wc == 0xc2);

  /* Encoder */
  strcpy(s, "..");
  assert(charset_wctomb(charset, s, 256) == -1 && !strcmp(s, ".."));
  assert(charset_wctomb(charset, s, 255) == 1 && !strcmp(s, "\377."));
  assert(charset_wctomb(charset, s, 128) == 1 && !strcmp(s, "\200."));
}

void test_iso2()
{
  struct charset *charset;
  int wc;
  char s[3];

  charset = charset_find("iso-8859-2");
  test_any(charset);

  /* Decoder */
  wc = 0;
  assert(charset_mbtowc(charset, &wc, "\302\200", 9) == 1 && wc == 0xc2);
  assert(charset_mbtowc(charset, &wc, "\377", 2) == 1 && wc == 0x2d9);

  /* Encoder */
  strcpy(s, "..");
  assert(charset_wctomb(charset, s, 256) == -1 && !strcmp(s, ".."));
  assert(charset_wctomb(charset, s, 255) == -1 && !strcmp(s, ".."));
  assert(charset_wctomb(charset, s, 258) == 1 && !strcmp(s, "\303."));
  assert(charset_wctomb(charset, s, 128) == 1 && !strcmp(s, "\200."));
}

void test_convert()
{
  const char *p;
  char *q, *r;
  char s[256];
  size_t n, n2;
  int i;

  p = "\000x\302\200\375\277\277\277\277\277";
  assert(charset_convert("UTF-8", "UTF-8", p, 10, &q, &n) == 0 &&
	 n == 10 && !strcmp(p, q));
  assert(charset_convert("UTF-8", "UTF-8", "x\301\277y", 4, &q, &n) == 2 &&
	 n == 4 && !strcmp(q, "x##y"));
  assert(charset_convert("UTF-8", "UTF-8", "x\301\277y", 4, 0, &n) == 2 &&
	 n == 4);
  assert(charset_convert("UTF-8", "UTF-8", "x\301\277y", 4, &q, 0) == 2 &&
	 !strcmp(q, "x##y"));
  assert(charset_convert("UTF-8", "iso-8859-1",
			 "\302\200\304\200x", 5, &q, &n) == 1 &&
	 n == 3 && !strcmp(q, "\200?x"));
  assert(charset_convert("iso-8859-1", "UTF-8", 
			 "\000\200\377", 3, &q, &n) == 0 &&
	 n == 5 && !memcmp(q, "\000\302\200\303\277", 5));
  assert(charset_convert("iso-8859-1", "iso-8859-1",
			 "\000\200\377", 3, &q, &n) == 0 &&
	 n == 3 && !memcmp(q, "\000\200\377", 3));

  assert(charset_convert("iso-8859-2", "utf-8", "\300", 1, &q, &n) == 0 &&
	 n == 2 && !strcmp(q, "\305\224"));
  assert(charset_convert("utf-8", "iso-8859-2", "\305\224", 2, &q, &n) == 0 &&
	 n == 1 && !strcmp(q, "\300"));

  for (i = 0; i < 256; i++)
    s[i] = i;

  assert(charset_convert("iso-8859-2", "utf-8", s, 256, &q, &n) == 0);
  assert(charset_convert("utf-8", "iso-8859-2", q, n, &r, &n2) == 0);
  assert(n2 == 256 && !memcmp(r, s, n2));
}

int main()
{
  test_utf8();
  test_ascii();
  test_iso1();
  test_iso2();

  test_convert();

  return 0;
}
