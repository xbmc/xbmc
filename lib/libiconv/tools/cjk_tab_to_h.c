/* Copyright (C) 1999-2004, 2006-2007 Free Software Foundation, Inc.
   This file is part of the GNU LIBICONV Tools.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/*
 * Generates a CJK character set table from a .TXT table as found on
 * ftp.unicode.org or in the X nls directory.
 * Examples:
 *
 *   ./cjk_tab_to_h GB2312.1980-0 gb2312 > gb2312.h < gb2312
 *   ./cjk_tab_to_h JISX0208.1983-0 jisx0208 > jisx0208.h < jis0208
 *   ./cjk_tab_to_h KSC5601.1987-0 ksc5601 > ksc5601.h < ksc5601
 *
 *   ./cjk_tab_to_h GB2312.1980-0 gb2312 > gb2312.h < GB2312.TXT
 *   ./cjk_tab_to_h JISX0208.1983-0 jisx0208 > jisx0208.h < JIS0208.TXT
 *   ./cjk_tab_to_h JISX0212.1990-0 jisx0212 > jisx0212.h < JIS0212.TXT
 *   ./cjk_tab_to_h KSC5601.1987-0 ksc5601 > ksc5601.h < KSC5601.TXT
 *   ./cjk_tab_to_h KSX1001.1992-0 ksc5601 > ksc5601.h < KSX1001.TXT
 *
 *   ./cjk_tab_to_h BIG5 big5 > big5.h < BIG5.TXT
 *
 *   ./cjk_tab_to_h JOHAB johab > johab.h < JOHAB.TXT
 *
 *   ./cjk_tab_to_h JISX0213:2004 jisx0213 > jisx0213.h < JISX0213.TXT
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

typedef struct {
  int start;
  int end;
} Block;

typedef struct {
  int rows;    /* number of possible values for the 1st byte */
  int cols;    /* number of possible values for the 2nd byte */
  int (*row_byte) (int row); /* returns the 1st byte value for a given row */
  int (*col_byte) (int col); /* returns the 2nd byte value for a given col */
  int (*byte_row) (int byte); /* converts a 1st byte value to a row, else -1 */
  int (*byte_col) (int byte); /* converts a 2nd byte value to a col, else -1 */
  const char* check_row_expr; /* format string for 1st byte value checking */
  const char* check_col_expr; /* format string for 2nd byte value checking */
  const char* byte_row_expr; /* format string for 1st byte value to row */
  const char* byte_col_expr; /* format string for 2nd byte value to col */
  int** charset2uni; /* charset2uni[0..rows-1][0..cols-1] is valid */
  /* You'll understand the terms "row" and "col" when you buy Ken Lunde's book.
     Once a row is fixed, choosing a "col" is the same as choosing a "cell". */
  int* charsetpage; /* charsetpage[0..rows]: how large is a page for a row */
  int ncharsetblocks;
  Block* charsetblocks; /* blocks[0..nblocks-1] */
  int* uni2charset; /* uni2charset[0x0000..0xffff] */
  int fffd;    /* uni representation of the invalid character */
} Encoding;

/*
 * Outputs the file title.
 */
static void output_title (const char *charsetname)
{
  printf("/*\n");
  printf(" * Copyright (C) 1999-2007 Free Software Foundation, Inc.\n");
  printf(" * This file is part of the GNU LIBICONV Library.\n");
  printf(" *\n");
  printf(" * The GNU LIBICONV Library is free software; you can redistribute it\n");
  printf(" * and/or modify it under the terms of the GNU Library General Public\n");
  printf(" * License as published by the Free Software Foundation; either version 2\n");
  printf(" * of the License, or (at your option) any later version.\n");
  printf(" *\n");
  printf(" * The GNU LIBICONV Library is distributed in the hope that it will be\n");
  printf(" * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
  printf(" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n");
  printf(" * Library General Public License for more details.\n");
  printf(" *\n");
  printf(" * You should have received a copy of the GNU Library General Public\n");
  printf(" * License along with the GNU LIBICONV Library; see the file COPYING.LIB.\n");
  printf(" * If not, write to the Free Software Foundation, Inc., 51 Franklin Street,\n");
  printf(" * Fifth Floor, Boston, MA 02110-1301, USA.\n");
  printf(" */\n");
  printf("\n");
  printf("/*\n");
  printf(" * %s\n", charsetname);
  printf(" */\n");
  printf("\n");
}

/*
 * Reads the charset2uni table from standard input.
 */
static void read_table (Encoding* enc)
{
  int row, col, i, i1, i2, c, j;

  enc->charset2uni = (int**) malloc(enc->rows*sizeof(int*));
  for (row = 0; row < enc->rows; row++)
    enc->charset2uni[row] = (int*) malloc(enc->cols*sizeof(int));

  for (row = 0; row < enc->rows; row++)
    for (col = 0; col < enc->cols; col++)
      enc->charset2uni[row][col] = 0xfffd;

  c = getc(stdin);
  ungetc(c,stdin);
  if (c == '#') {
    /* Read a unicode.org style .TXT file. */
    for (;;) {
      c = getc(stdin);
      if (c == EOF)
        break;
      if (c == '\n' || c == ' ' || c == '\t')
        continue;
      if (c == '#') {
        do { c = getc(stdin); } while (!(c == EOF || c == '\n'));
        continue;
      }
      ungetc(c,stdin);
      if (scanf("0x%x", &j) != 1)
        exit(1);
      i1 = j >> 8;
      i2 = j & 0xff;
      row = enc->byte_row(i1);
      col = enc->byte_col(i2);
      if (row < 0 || col < 0) {
        fprintf(stderr, "lost entry for %02x %02x\n", i1, i2);
        exit(1);
      }
      if (scanf(" 0x%x", &enc->charset2uni[row][col]) != 1)
        exit(1);
    }
  } else {
    /* Read a table of hexadecimal Unicode values. */
    for (i1 = 32; i1 < 132; i1++)
      for (i2 = 32; i2 < 132; i2++) {
        i = scanf("%x", &j);
        if (i == EOF)
          goto read_done;
        if (i != 1)
          exit(1);
        if (j < 0 || j == 0xffff)
          j = 0xfffd;
        if (j != 0xfffd) {
          if (enc->byte_row(i1) < 0 || enc->byte_col(i2) < 0) {
            fprintf(stderr, "lost entry at %02x %02x\n", i1, i2);
            exit (1);
          }
          enc->charset2uni[enc->byte_row(i1)][enc->byte_col(i2)] = j;
        }
      }
   read_done: ;
  }
}

/*
 * Determine whether the Unicode range goes outside the BMP.
 */
static bool is_charset2uni_large (Encoding* enc)
{
  int row, col;

  for (row = 0; row < enc->rows; row++)
    for (col = 0; col < enc->cols; col++)
      if (enc->charset2uni[row][col] >= 0x10000)
        return true;
  return false;
}

/*
 * Compactify the Unicode range by use of an auxiliary table,
 * so 16 bits suffice to store each value.
 */
static int compact_large_charset2uni (Encoding* enc, unsigned int **urows, unsigned int *urowshift)
{
  unsigned int shift;

  for (shift = 8; ; shift--) {
    int *upages = (int *) malloc((0x110000>>shift) * sizeof(int));
    int i, row, col, nurows;

    for (i = 0; i < 0x110000>>shift; i++)
      upages[i] = -1;

    for (row = 0; row < enc->rows; row++)
      for (col = 0; col < enc->cols; col++)
        upages[enc->charset2uni[row][col] >> shift] = 0;

    nurows = 0;
    for (i = 0; i < 0x110000>>shift; i++)
      if (upages[i] == 0)
        nurows++;

    /* We want all table entries to fit in an 'unsigned short'. */
    if (nurows <= 1<<(16-shift)) {
      int** old_charset2uni;

      *urows = (unsigned int *) malloc(nurows * sizeof(unsigned int));
      *urowshift = shift;

      nurows = 0;
      for (i = 0; i < 0x110000>>shift; i++)
        if (upages[i] == 0) {
          upages[i] = nurows;
          (*urows)[nurows] = i;
          nurows++;
        }

      old_charset2uni = enc->charset2uni;
      enc->charset2uni = (int**) malloc(enc->rows*sizeof(int*));
      for (row = 0; row < enc->rows; row++)
        enc->charset2uni[row] = (int*) malloc(enc->cols*sizeof(int));
      for (row = 0; row < enc->rows; row++)
        for (col = 0; col < enc->cols; col++) {
          int u = old_charset2uni[row][col];
          enc->charset2uni[row][col] =
            (upages[u >> shift] << shift) | (u & ((1 << shift) - 1));
        }
      enc->fffd =
        (upages[0xfffd >> shift] << shift) | (0xfffd & ((1 << shift) - 1));

      return nurows;
    }
  }
  abort();
}

/*
 * Computes the charsetpage[0..rows] array.
 */
static void find_charset2uni_pages (Encoding* enc)
{
  int row, col;

  enc->charsetpage = (int*) malloc((enc->rows+1)*sizeof(int));

  for (row = 0; row <= enc->rows; row++)
    enc->charsetpage[row] = 0;

  for (row = 0; row < enc->rows; row++) {
    int used = 0;
    for (col = 0; col < enc->cols; col++)
      if (enc->charset2uni[row][col] != enc->fffd)
        used = col+1;
    enc->charsetpage[row] = used;
  }
}

/*
 * Fills in nblocks and blocks.
 */
static void find_charset2uni_blocks (Encoding* enc)
{
  int n, row, lastrow;

  enc->charsetblocks = (Block*) malloc(enc->rows*sizeof(Block));

  n = 0;
  for (row = 0; row < enc->rows; row++)
    if (enc->charsetpage[row] > 0 && (row == 0 || enc->charsetpage[row-1] == 0)) {
      for (lastrow = row; enc->charsetpage[lastrow+1] > 0; lastrow++);
      enc->charsetblocks[n].start = row * enc->cols;
      enc->charsetblocks[n].end = lastrow * enc->cols + enc->charsetpage[lastrow];
      n++;
    }
  enc->ncharsetblocks = n;
}

/*
 * Outputs the charset to unicode table and function.
 */
static void output_charset2uni (const char* name, Encoding* enc)
{
  int nurows, row, col, lastrow, col_max, i, i1_min, i1_max;
  bool is_large;
  unsigned int* urows;
  unsigned int urowshift;
  Encoding tmpenc;

  is_large = is_charset2uni_large(enc);
  if (is_large) {
    /* Use a temporary copy of enc. */
    tmpenc = *enc;
    enc = &tmpenc;
    nurows = compact_large_charset2uni(enc,&urows,&urowshift);
  } else {
    nurows = 0; urows = NULL; urowshift = 0; enc->fffd = 0xfffd;
  }

  find_charset2uni_pages(enc);

  find_charset2uni_blocks(enc);

  for (row = 0; row < enc->rows; row++)
    if (enc->charsetpage[row] > 0) {
      if (row == 0 || enc->charsetpage[row-1] == 0) {
        /* Start a new block. */
        for (lastrow = row; enc->charsetpage[lastrow+1] > 0; lastrow++);
        printf("static const unsigned short %s_2uni_page%02x[%d] = {\n",
               name, enc->row_byte(row),
               (lastrow-row) * enc->cols + enc->charsetpage[lastrow]);
      }
      printf("  /""* 0x%02x *""/\n ", enc->row_byte(row));
      col_max = (enc->charsetpage[row+1] > 0 ? enc->cols : enc->charsetpage[row]);
      for (col = 0; col < col_max; col++) {
        printf(" 0x%04x,", enc->charset2uni[row][col]);
        if ((col % 8) == 7 && (col+1 < col_max)) printf("\n ");
      }
      printf("\n");
      if (enc->charsetpage[row+1] == 0) {
        /* End a block. */
        printf("};\n");
      }
    }
  printf("\n");

  if (is_large) {
    printf("static const ucs4_t %s_2uni_upages[%d] = {\n ", name, nurows);
    for (i = 0; i < nurows; i++) {
      printf(" 0x%05x,", urows[i] << urowshift);
      if ((i % 8) == 7 && (i+1 < nurows)) printf("\n ");
    }
    printf("\n");
    printf("};\n");
    printf("\n");
  }

  printf("static int\n");
  printf("%s_mbtowc (conv_t conv, ucs4_t *pwc, const unsigned char *s, int n)\n", name);
  printf("{\n");
  printf("  unsigned char c1 = s[0];\n");
  printf("  if (");
  for (i = 0; i < enc->ncharsetblocks; i++) {
    i1_min = enc->row_byte(enc->charsetblocks[i].start / enc->cols);
    i1_max = enc->row_byte((enc->charsetblocks[i].end-1) / enc->cols);
    if (i > 0)
      printf(" || ");
    if (i1_min == i1_max)
      printf("(c1 == 0x%02x)", i1_min);
    else
      printf("(c1 >= 0x%02x && c1 <= 0x%02x)", i1_min, i1_max);
  }
  printf(") {\n");
  printf("    if (n >= 2) {\n");
  printf("      unsigned char c2 = s[1];\n");
  printf("      if (");
  printf(enc->check_col_expr, "c2");
  printf(") {\n");
  printf("        unsigned int i = %d * (", enc->cols);
  printf(enc->byte_row_expr, "c1");
  printf(") + (");
  printf(enc->byte_col_expr, "c2");
  printf(");\n");
  printf("        %s wc = 0xfffd;\n", is_large ? "ucs4_t" : "unsigned short");
  if (is_large) printf("        unsigned short swc;\n");
  for (i = 0; i < enc->ncharsetblocks; i++) {
    printf("        ");
    if (i > 0)
      printf("} else ");
    if (i < enc->ncharsetblocks-1)
      printf("if (i < %d) ", enc->charsetblocks[i+1].start);
    printf("{\n");
    printf("          if (i < %d)\n", enc->charsetblocks[i].end);
    printf("            %s = ", is_large ? "swc" : "wc");
    printf("%s_2uni_page%02x[i", name, enc->row_byte(enc->charsetblocks[i].start / enc->cols));
    if (enc->charsetblocks[i].start > 0)
      printf("-%d", enc->charsetblocks[i].start);
    printf("]");
    if (is_large) printf(",\n            wc = %s_2uni_upages[swc>>%d] | (swc & 0x%x)", name, urowshift, (1 << urowshift) - 1);
    printf(";\n");
  }
  printf("        }\n");
  printf("        if (wc != 0xfffd) {\n");
  printf("          *pwc = %swc;\n", is_large ? "" : "(ucs4_t) ");
  printf("          return 2;\n");
  printf("        }\n");
  printf("      }\n");
  printf("      return RET_ILSEQ;\n");
  printf("    }\n");
  printf("    return RET_TOOFEW(0);\n");
  printf("  }\n");
  printf("  return RET_ILSEQ;\n");
  printf("}\n");
  printf("\n");
}

/*
 * Outputs the charset to unicode table and function.
 * (Suitable if the mapping function is well defined, i.e. has no holes, and
 * is monotonically increasing with small gaps only.)
 */
static void output_charset2uni_noholes_monotonic (const char* name, Encoding* enc)
{
  int row, col, lastrow, r, col_max, i, i1_min, i1_max;

  /* Choose stepsize so that stepsize*steps_per_row >= enc->cols, and
     enc->charset2uni[row][col] - enc->charset2uni[row][col/stepsize*stepsize]
     is always < 0x100. */
  int steps_per_row = 2;
  int stepsize = (enc->cols + steps_per_row-1) / steps_per_row;

  find_charset2uni_pages(enc);

  find_charset2uni_blocks(enc);

  for (row = 0; row < enc->rows; row++)
    if (enc->charsetpage[row] > 0) {
      if (row == 0 || enc->charsetpage[row-1] == 0) {
        /* Start a new block. */
        for (lastrow = row; enc->charsetpage[lastrow+1] > 0; lastrow++);
        printf("static const unsigned short %s_2uni_main_page%02x[%d] = {\n ",
               name, enc->row_byte(row),
               steps_per_row*(lastrow-row+1));
        for (r = row; r <= lastrow; r++) {
          for (i = 0; i < steps_per_row; i++)
            printf(" 0x%04x,", enc->charset2uni[r][i*stepsize]);
          if (((r-row) % 4) == 3 && (r < lastrow)) printf("\n ");
        }
        printf("\n");
        printf("};\n");
        printf("static const unsigned char %s_2uni_page%02x[%d] = {\n",
               name, enc->row_byte(row),
               (lastrow-row) * enc->cols + enc->charsetpage[lastrow]);
      }
      printf("  /""* 0x%02x *""/\n ", enc->row_byte(row));
      col_max = (enc->charsetpage[row+1] > 0 ? enc->cols : enc->charsetpage[row]);
      for (col = 0; col < col_max; col++) {
        printf(" 0x%02x,", enc->charset2uni[row][col] - enc->charset2uni[row][col/stepsize*stepsize]);
        if ((col % 8) == 7 && (col+1 < col_max)) printf("\n ");
      }
      printf("\n");
      if (enc->charsetpage[row+1] == 0) {
        /* End a block. */
        printf("};\n");
      }
    }
  printf("\n");

  printf("static int\n");
  printf("%s_mbtowc (conv_t conv, ucs4_t *pwc, const unsigned char *s, int n)\n", name);
  printf("{\n");
  printf("  unsigned char c1 = s[0];\n");
  printf("  if (");
  for (i = 0; i < enc->ncharsetblocks; i++) {
    i1_min = enc->row_byte(enc->charsetblocks[i].start / enc->cols);
    i1_max = enc->row_byte((enc->charsetblocks[i].end-1) / enc->cols);
    if (i > 0)
      printf(" || ");
    if (i1_min == i1_max)
      printf("(c1 == 0x%02x)", i1_min);
    else
      printf("(c1 >= 0x%02x && c1 <= 0x%02x)", i1_min, i1_max);
  }
  printf(") {\n");
  printf("    if (n >= 2) {\n");
  printf("      unsigned char c2 = s[1];\n");
  printf("      if (");
  printf(enc->check_col_expr, "c2");
  printf(") {\n");
  printf("        unsigned int row = ");
  printf(enc->byte_row_expr, "c1");
  printf(";\n");
  printf("        unsigned int col = ");
  printf(enc->byte_col_expr, "c2");
  printf(";\n");
  printf("        unsigned int i = %d * row + col;\n", enc->cols);
  printf("        unsigned short wc = 0xfffd;\n");
  for (i = 0; i < enc->ncharsetblocks; i++) {
    printf("        ");
    if (i > 0)
      printf("} else ");
    if (i < enc->ncharsetblocks-1)
      printf("if (i < %d) ", enc->charsetblocks[i+1].start);
    printf("{\n");
    printf("          if (i < %d)\n", enc->charsetblocks[i].end);
    printf("            wc = %s_2uni_main_page%02x[%d*", name, enc->row_byte(enc->charsetblocks[i].start / enc->cols), steps_per_row);
    if (enc->charsetblocks[i].start > 0)
      printf("(row-%d)", enc->charsetblocks[i].start / enc->cols);
    else
      printf("row");
    printf("+");
    if (steps_per_row == 2)
      printf("(col>=%d?1:0)", stepsize);
    else
      printf("col/%d", stepsize);
    printf("] + %s_2uni_page%02x[i", name, enc->row_byte(enc->charsetblocks[i].start / enc->cols));
    if (enc->charsetblocks[i].start > 0)
      printf("-%d", enc->charsetblocks[i].start);
    printf("];\n");
  }
  printf("        }\n");
  printf("        if (wc != 0xfffd) {\n");
  printf("          *pwc = (ucs4_t) wc;\n");
  printf("          return 2;\n");
  printf("        }\n");
  printf("      }\n");
  printf("      return RET_ILSEQ;\n");
  printf("    }\n");
  printf("    return RET_TOOFEW(0);\n");
  printf("  }\n");
  printf("  return RET_ILSEQ;\n");
  printf("}\n");
  printf("\n");
}

/*
 * Computes the uni2charset[0x0000..0x2ffff] array.
 */
static void invert (Encoding* enc)
{
  int row, col, j;

  enc->uni2charset = (int*) malloc(0x30000*sizeof(int));

  for (j = 0; j < 0x30000; j++)
    enc->uni2charset[j] = 0;

  for (row = 0; row < enc->rows; row++)
    for (col = 0; col < enc->cols; col++) {
      j = enc->charset2uni[row][col];
      if (j != 0xfffd)
        enc->uni2charset[j] = 0x100 * enc->row_byte(row) + enc->col_byte(col);
    }
}

/*
 * Outputs the unicode to charset table and function, using a linear array.
 * (Suitable if the table is dense.)
 */
static void output_uni2charset_dense (const char* name, Encoding* enc)
{
  /* Like in 8bit_tab_to_h.c */
  bool pages[0x300];
  int line[0x6000];
  int tableno;
  struct { int minline; int maxline; int usecount; } tables[0x6000];
  bool first;
  int row, col, j, p, j1, j2, t;

  for (p = 0; p < 0x300; p++)
    pages[p] = false;
  for (row = 0; row < enc->rows; row++)
    for (col = 0; col < enc->cols; col++) {
      j = enc->charset2uni[row][col];
      if (j != 0xfffd)
        pages[j>>8] = true;
    }
  for (j1 = 0; j1 < 0x6000; j1++) {
    bool all_invalid = true;
    for (j2 = 0; j2 < 8; j2++) {
      j = 8*j1+j2;
      if (enc->uni2charset[j] != 0)
        all_invalid = false;
    }
    if (all_invalid)
      line[j1] = -1;
    else
      line[j1] = 0;
  }
  tableno = 0;
  for (j1 = 0; j1 < 0x6000; j1++) {
    if (line[j1] >= 0) {
      if (tableno > 0
          && ((j1 > 0 && line[j1-1] == tableno-1)
              || ((tables[tableno-1].maxline >> 5) == (j1 >> 5)
                  && j1 - tables[tableno-1].maxline <= 8))) {
        line[j1] = tableno-1;
        tables[tableno-1].maxline = j1;
      } else {
        tableno++;
        line[j1] = tableno-1;
        tables[tableno-1].minline = tables[tableno-1].maxline = j1;
      }
    }
  }
  for (t = 0; t < tableno; t++) {
    tables[t].usecount = 0;
    j1 = 8*tables[t].minline;
    j2 = 8*(tables[t].maxline+1);
    for (j = j1; j < j2; j++)
      if (enc->uni2charset[j] != 0)
        tables[t].usecount++;
  }
  {
    p = -1;
    for (t = 0; t < tableno; t++)
      if (tables[t].usecount > 1) {
        p = tables[t].minline >> 5;
        printf("static const unsigned short %s_page%02x[%d] = {\n", name, p, 8*(tables[t].maxline-tables[t].minline+1));
        for (j1 = tables[t].minline; j1 <= tables[t].maxline; j1++) {
          if ((j1 % 0x20) == 0 && j1 > tables[t].minline)
            printf("  /* 0x%04x */\n", 8*j1);
          printf(" ");
          for (j2 = 0; j2 < 8; j2++) {
            j = 8*j1+j2;
            printf(" 0x%04x,", enc->uni2charset[j]);
          }
          printf(" /*0x%02x-0x%02x*/\n", 8*(j1 % 0x20), 8*(j1 % 0x20)+7);
        }
        printf("};\n");
      }
    if (p >= 0)
      printf("\n");
  }
  printf("static int\n%s_wctomb (conv_t conv, unsigned char *r, ucs4_t wc, int n)\n", name);
  printf("{\n");
  printf("  if (n >= 2) {\n");
  printf("    unsigned short c = 0;\n");
  first = true;
  for (j1 = 0; j1 < 0x6000;) {
    t = line[j1];
    for (j2 = j1; j2 < 0x6000 && line[j2] == t; j2++);
    if (t >= 0) {
      if (j1 != tables[t].minline) abort();
      if (j2 > tables[t].maxline+1) abort();
      j2 = tables[t].maxline+1;
      if (first)
        printf("    ");
      else
        printf("    else ");
      first = false;
      if (tables[t].usecount == 0) abort();
      if (tables[t].usecount == 1) {
        if (j2 != j1+1) abort();
        for (j = 8*j1; j < 8*j2; j++)
          if (enc->uni2charset[j] != 0) {
            printf("if (wc == 0x%04x)\n      c = 0x%02x;\n", j, enc->uni2charset[j]);
            break;
          }
      } else {
        if (j1 == 0) {
          printf("if (wc < 0x%04x)", 8*j2);
        } else {
          printf("if (wc >= 0x%04x && wc < 0x%04x)", 8*j1, 8*j2);
        }
        printf("\n      c = %s_page%02x[wc", name, j1 >> 5);
        if (tables[t].minline > 0)
          printf("-0x%04x", 8*j1);
        printf("];\n");
      }
    }
    j1 = j2;
  }
  printf("    if (c != 0) {\n");
  printf("      r[0] = (c >> 8); r[1] = (c & 0xff);\n");
  printf("      return 2;\n");
  printf("    }\n");
  printf("    return RET_ILUNI;\n");
  printf("  }\n");
  printf("  return RET_TOOSMALL;\n");
  printf("}\n");
}

/*
 * Outputs the unicode to charset table and function, using a packed array.
 * (Suitable if the table is sparse.)
 * The argument 'monotonic' may be set to true if the mapping is monotonically
 * increasing with small gaps only.
 */
static void output_uni2charset_sparse (const char* name, Encoding* enc, bool monotonic)
{
  bool pages[0x300];
  Block pageblocks[0x300]; int npageblocks;
  int indx2charset[0x30000];
  int summary_indx[0x3000];
  int summary_used[0x3000];
  int i, row, col, j, p, j1, j2, indx;
  bool is_large;
  /* for monotonic: */
  int log2_stepsize = (!strcmp(name,"uhc_2") ? 6 : 7);
  int stepsize = 1 << log2_stepsize;
  int indxsteps;

  /* Fill pages[0x300]. */
  for (p = 0; p < 0x300; p++)
    pages[p] = false;
  for (row = 0; row < enc->rows; row++)
    for (col = 0; col < enc->cols; col++) {
      j = enc->charset2uni[row][col];
      if (j != 0xfffd)
        pages[j>>8] = true;
    }

  /* Determine whether two or three bytes are needed for each character. */
  is_large = false;
  for (j = 0; j < 0x30000; j++)
    if (enc->uni2charset[j] >= 0x10000)
      is_large = true;

#if 0
  for (p = 0; p < 0x300; p++)
    if (pages[p]) {
      printf("static const unsigned short %s_page%02x[256] = {\n", name, p);
      for (j1 = 0; j1 < 32; j1++) {
        printf("  ");
        for (j2 = 0; j2 < 8; j2++)
          printf("0x%04x, ", enc->uni2charset[256*p+8*j1+j2]);
        printf("/""*0x%02x-0x%02x*""/\n", 8*j1, 8*j1+7);
      }
      printf("};\n");
    }
  printf("\n");
#endif

  /* Fill summary_indx[] and summary_used[]. */
  indx = 0;
  for (j1 = 0; j1 < 0x3000; j1++) {
    summary_indx[j1] = indx;
    summary_used[j1] = 0;
    for (j2 = 0; j2 < 16; j2++) {
      j = 16*j1+j2;
      if (enc->uni2charset[j] != 0) {
        indx2charset[indx++] = enc->uni2charset[j];
        summary_used[j1] |= (1 << j2);
      }
    }
  }

  /* Fill npageblocks and pageblocks[]. */
  npageblocks = 0;
  for (p = 0; p < 0x300; ) {
    if (pages[p] && (p == 0 || !pages[p-1])) {
      pageblocks[npageblocks].start = 16*p;
      do p++; while (p < 0x300 && pages[p]);
      j1 = 16*p;
      while (summary_used[j1-1] == 0) j1--;
      pageblocks[npageblocks].end = j1;
      npageblocks++;
    } else
      p++;
  }

  if (monotonic) {
    indxsteps = (indx + stepsize-1) / stepsize;
    printf("static const unsigned short %s_2charset_main[%d] = {\n", name, indxsteps);
    for (i = 0; i < indxsteps; ) {
      if ((i % 8) == 0) printf(" ");
      printf(" 0x%04x,", indx2charset[i*stepsize]);
      i++;
      if ((i % 8) == 0 || i == indxsteps) printf("\n");
    }
    printf("};\n");
    printf("static const unsigned char %s_2charset[%d] = {\n", name, indx);
    for (i = 0; i < indx; ) {
      if ((i % 8) == 0) printf(" ");
      printf(" 0x%02x,", indx2charset[i] - indx2charset[i/stepsize*stepsize]);
      i++;
      if ((i % 8) == 0 || i == indx) printf("\n");
    }
    printf("};\n");
  } else {
    if (is_large) {
      printf("static const unsigned char %s_2charset[3*%d] = {\n", name, indx);
      for (i = 0; i < indx; ) {
        if ((i % 4) == 0) printf(" ");
        printf(" 0x%1x,0x%02x,0x%02x,", indx2charset[i] >> 16,
               (indx2charset[i] >> 8) & 0xff, indx2charset[i] & 0xff);
        i++;
        if ((i % 4) == 0 || i == indx) printf("\n");
      }
      printf("};\n");
    } else {
      printf("static const unsigned short %s_2charset[%d] = {\n", name, indx);
      for (i = 0; i < indx; ) {
        if ((i % 8) == 0) printf(" ");
        printf(" 0x%04x,", indx2charset[i]);
        i++;
        if ((i % 8) == 0 || i == indx) printf("\n");
      }
      printf("};\n");
    }
  }
  printf("\n");
  for (i = 0; i < npageblocks; i++) {
    printf("static const Summary16 %s_uni2indx_page%02x[%d] = {\n", name,
           pageblocks[i].start/16, pageblocks[i].end-pageblocks[i].start);
    for (j1 = pageblocks[i].start; j1 < pageblocks[i].end; ) {
      if (((16*j1) % 0x100) == 0) printf("  /""* 0x%04x *""/\n", 16*j1);
      if ((j1 % 4) == 0) printf(" ");
      printf(" { %4d, 0x%04x },", summary_indx[j1], summary_used[j1]);
      j1++;
      if ((j1 % 4) == 0 || j1 == pageblocks[i].end) printf("\n");
    }
    printf("};\n");
  }
  printf("\n");

  printf("static int\n");
  printf("%s_wctomb (conv_t conv, unsigned char *r, ucs4_t wc, int n)\n", name);
  printf("{\n");
  printf("  if (n >= 2) {\n");
  printf("    const Summary16 *summary = NULL;\n");
  for (i = 0; i < npageblocks; i++) {
    printf("    ");
    if (i > 0)
      printf("else ");
    printf("if (wc >= 0x%04x && wc < 0x%04x)\n",
           16*pageblocks[i].start, 16*pageblocks[i].end);
    printf("      summary = &%s_uni2indx_page%02x[(wc>>4)", name,
           pageblocks[i].start/16);
    if (pageblocks[i].start > 0)
      printf("-0x%03x", pageblocks[i].start);
    printf("];\n");
  }
  printf("    if (summary) {\n");
  printf("      unsigned short used = summary->used;\n");
  printf("      unsigned int i = wc & 0x0f;\n");
  printf("      if (used & ((unsigned short) 1 << i)) {\n");
  if (monotonic || !is_large)
    printf("        unsigned short c;\n");
  printf("        /* Keep in `used' only the bits 0..i-1. */\n");
  printf("        used &= ((unsigned short) 1 << i) - 1;\n");
  printf("        /* Add `summary->indx' and the number of bits set in `used'. */\n");
  printf("        used = (used & 0x5555) + ((used & 0xaaaa) >> 1);\n");
  printf("        used = (used & 0x3333) + ((used & 0xcccc) >> 2);\n");
  printf("        used = (used & 0x0f0f) + ((used & 0xf0f0) >> 4);\n");
  printf("        used = (used & 0x00ff) + (used >> 8);\n");
  if (monotonic) {
    printf("        used += summary->indx;\n");
    printf("        c = %s_2charset_main[used>>%d] + %s_2charset[used];\n", name, log2_stepsize, name);
    printf("        r[0] = (c >> 8); r[1] = (c & 0xff);\n");
    printf("        return 2;\n");
  } else {
    if (is_large) {
      printf("        used += summary->indx;\n");
      printf("        r[0] = %s_2charset[3*used];\n", name);
      printf("        r[1] = %s_2charset[3*used+1];\n", name);
      printf("        r[2] = %s_2charset[3*used+2];\n", name);
      printf("        return 3;\n");
    } else {
      printf("        c = %s_2charset[summary->indx + used];\n", name);
      printf("        r[0] = (c >> 8); r[1] = (c & 0xff);\n");
      printf("        return 2;\n");
    }
  }
  printf("      }\n");
  printf("    }\n");
  printf("    return RET_ILUNI;\n");
  printf("  }\n");
  printf("  return RET_TOOSMALL;\n");
  printf("}\n");
}

/* ISO-2022/EUC specifics */

static int row_byte_normal (int row) { return 0x21+row; }
static int col_byte_normal (int col) { return 0x21+col; }
static int byte_row_normal (int byte) { return byte-0x21; }
static int byte_col_normal (int byte) { return byte-0x21; }

static void do_normal (const char* name)
{
  Encoding enc;

  enc.rows = 94;
  enc.cols = 94;
  enc.row_byte = row_byte_normal;
  enc.col_byte = col_byte_normal;
  enc.byte_row = byte_row_normal;
  enc.byte_col = byte_col_normal;
  enc.check_row_expr = "%1$s >= 0x21 && %1$s < 0x7f";
  enc.check_col_expr = "%1$s >= 0x21 && %1$s < 0x7f";
  enc.byte_row_expr = "%1$s - 0x21";
  enc.byte_col_expr = "%1$s - 0x21";

  read_table(&enc);
  output_charset2uni(name,&enc);
  invert(&enc); output_uni2charset_sparse(name,&enc,false);
}

/* Note: On first sight, the jisx0212_2charset[] table seems to be in order,
   starting from the charset=0x3021/uni=0x4e02 pair. But it's only mostly in
   order. There are 75 out-of-order values, scattered all throughout the table.
 */

static void do_normal_only_charset2uni (const char* name)
{
  Encoding enc;

  enc.rows = 94;
  enc.cols = 94;
  enc.row_byte = row_byte_normal;
  enc.col_byte = col_byte_normal;
  enc.byte_row = byte_row_normal;
  enc.byte_col = byte_col_normal;
  enc.check_row_expr = "%1$s >= 0x21 && %1$s < 0x7f";
  enc.check_col_expr = "%1$s >= 0x21 && %1$s < 0x7f";
  enc.byte_row_expr = "%1$s - 0x21";
  enc.byte_col_expr = "%1$s - 0x21";

  read_table(&enc);
  output_charset2uni(name,&enc);
}

/* CNS 11643 specifics - trick to put two tables into one */

static int row_byte_cns11643 (int row) {
  return 0x100 * (row / 94) + (row % 94) + 0x21;
}
static int byte_row_cns11643 (int byte) {
  return (byte >> 8) * 94 + (byte & 0xff) - 0x21;
}

static void do_cns11643_only_uni2charset (const char* name)
{
  Encoding enc;

  enc.rows = 16*94;
  enc.cols = 94;
  enc.row_byte = row_byte_cns11643;
  enc.col_byte = col_byte_normal;
  enc.byte_row = byte_row_cns11643;
  enc.byte_col = byte_col_normal;
  enc.check_row_expr = "%1$s >= 0x21 && %1$s < 0x7f";
  enc.check_col_expr = "%1$s >= 0x21 && %1$s < 0x7f";
  enc.byte_row_expr = "%1$s - 0x21";
  enc.byte_col_expr = "%1$s - 0x21";

  read_table(&enc);
  invert(&enc);
  output_uni2charset_sparse(name,&enc,false);
}

/* GBK specifics */

static int row_byte_gbk1 (int row) {
  return 0x81+row;
}
static int col_byte_gbk1 (int col) {
  return (col >= 0x3f ? 0x41 : 0x40) + col;
}
static int byte_row_gbk1 (int byte) {
  if (byte >= 0x81 && byte < 0xff)
    return byte-0x81;
  else
    return -1;
}
static int byte_col_gbk1 (int byte) {
  if (byte >= 0x40 && byte < 0x7f)
    return byte-0x40;
  else if (byte >= 0x80 && byte < 0xff)
    return byte-0x41;
  else
    return -1;
}

static void do_gbk1 (const char* name)
{
  Encoding enc;

  enc.rows = 126;
  enc.cols = 190;
  enc.row_byte = row_byte_gbk1;
  enc.col_byte = col_byte_gbk1;
  enc.byte_row = byte_row_gbk1;
  enc.byte_col = byte_col_gbk1;
  enc.check_row_expr = "%1$s >= 0x81 && %1$s < 0xff";
  enc.check_col_expr = "(%1$s >= 0x40 && %1$s < 0x7f) || (%1$s >= 0x80 && %1$s < 0xff)";
  enc.byte_row_expr = "%1$s - 0x81";
  enc.byte_col_expr = "%1$s - (%1$s >= 0x80 ? 0x41 : 0x40)";

  read_table(&enc);
  output_charset2uni(name,&enc);
  invert(&enc); output_uni2charset_dense(name,&enc);
}

static void do_gbk1_only_charset2uni (const char* name)
{
  Encoding enc;

  enc.rows = 126;
  enc.cols = 190;
  enc.row_byte = row_byte_gbk1;
  enc.col_byte = col_byte_gbk1;
  enc.byte_row = byte_row_gbk1;
  enc.byte_col = byte_col_gbk1;
  enc.check_row_expr = "%1$s >= 0x81 && %1$s < 0xff";
  enc.check_col_expr = "(%1$s >= 0x40 && %1$s < 0x7f) || (%1$s >= 0x80 && %1$s < 0xff)";
  enc.byte_row_expr = "%1$s - 0x81";
  enc.byte_col_expr = "%1$s - (%1$s >= 0x80 ? 0x41 : 0x40)";

  read_table(&enc);
  output_charset2uni(name,&enc);
}

static int row_byte_gbk2 (int row) {
  return 0x81+row;
}
static int col_byte_gbk2 (int col) {
  return (col >= 0x3f ? 0x41 : 0x40) + col;
}
static int byte_row_gbk2 (int byte) {
  if (byte >= 0x81 && byte < 0xff)
    return byte-0x81;
  else
    return -1;
}
static int byte_col_gbk2 (int byte) {
  if (byte >= 0x40 && byte < 0x7f)
    return byte-0x40;
  else if (byte >= 0x80 && byte < 0xa1)
    return byte-0x41;
  else
    return -1;
}

static void do_gbk2_only_charset2uni (const char* name)
{
  Encoding enc;

  enc.rows = 126;
  enc.cols = 96;
  enc.row_byte = row_byte_gbk2;
  enc.col_byte = col_byte_gbk2;
  enc.byte_row = byte_row_gbk2;
  enc.byte_col = byte_col_gbk2;
  enc.check_row_expr = "%1$s >= 0x81 && %1$s < 0xff";
  enc.check_col_expr = "(%1$s >= 0x40 && %1$s < 0x7f) || (%1$s >= 0x80 && %1$s < 0xa1)";
  enc.byte_row_expr = "%1$s - 0x81";
  enc.byte_col_expr = "%1$s - (%1$s >= 0x80 ? 0x41 : 0x40)";

  read_table(&enc);
  output_charset2uni(name,&enc);
}

static void do_gbk1_only_uni2charset (const char* name)
{
  Encoding enc;

  enc.rows = 126;
  enc.cols = 190;
  enc.row_byte = row_byte_gbk1;
  enc.col_byte = col_byte_gbk1;
  enc.byte_row = byte_row_gbk1;
  enc.byte_col = byte_col_gbk1;
  enc.check_row_expr = "%1$s >= 0x81 && %1$s < 0xff";
  enc.check_col_expr = "(%1$s >= 0x40 && %1$s < 0x7f) || (%1$s >= 0x80 && %1$s < 0xff)";
  enc.byte_row_expr = "%1$s - 0x81";
  enc.byte_col_expr = "%1$s - (%1$s >= 0x80 ? 0x41 : 0x40)";

  read_table(&enc);
  invert(&enc); output_uni2charset_sparse(name,&enc,false);
}

/* KSC 5601 specifics */

/*
 * Reads the charset2uni table from standard input.
 */
static void read_table_ksc5601 (Encoding* enc)
{
  int row, col, i, i1, i2, c, j;

  enc->charset2uni = (int**) malloc(enc->rows*sizeof(int*));
  for (row = 0; row < enc->rows; row++)
    enc->charset2uni[row] = (int*) malloc(enc->cols*sizeof(int));

  for (row = 0; row < enc->rows; row++)
    for (col = 0; col < enc->cols; col++)
      enc->charset2uni[row][col] = 0xfffd;

  c = getc(stdin);
  ungetc(c,stdin);
  if (c == '#') {
    /* Read a unicode.org style .TXT file. */
    for (;;) {
      c = getc(stdin);
      if (c == EOF)
        break;
      if (c == '\n' || c == ' ' || c == '\t')
        continue;
      if (c == '#') {
        do { c = getc(stdin); } while (!(c == EOF || c == '\n'));
        continue;
      }
      ungetc(c,stdin);
      if (scanf("0x%x", &j) != 1)
        exit(1);
      i1 = j >> 8;
      i2 = j & 0xff;
      if (scanf(" 0x%x", &j) != 1)
        exit(1);
      /* Take only the range covered by KS C 5601.1987-0 = KS C 5601.1989-0
         = KS X 1001.1992, ignore the rest. */
      if (!(i1 >= 128+33 && i1 < 128+127 && i2 >= 128+33 && i2 < 128+127))
        continue;  /* KSC5601 specific */
      i1 &= 0x7f;  /* KSC5601 specific */
      i2 &= 0x7f;  /* KSC5601 specific */
      row = enc->byte_row(i1);
      col = enc->byte_col(i2);
      if (row < 0 || col < 0) {
        fprintf(stderr, "lost entry for %02x %02x\n", i1, i2);
        exit(1);
      }
      enc->charset2uni[row][col] = j;
    }
  } else {
    /* Read a table of hexadecimal Unicode values. */
    for (i1 = 33; i1 < 127; i1++)
      for (i2 = 33; i2 < 127; i2++) {
        i = scanf("%x", &j);
        if (i == EOF)
          goto read_done;
        if (i != 1)
          exit(1);
        if (j < 0 || j == 0xffff)
          j = 0xfffd;
        if (j != 0xfffd) {
          if (enc->byte_row(i1) < 0 || enc->byte_col(i2) < 0) {
            fprintf(stderr, "lost entry at %02x %02x\n", i1, i2);
            exit (1);
          }
          enc->charset2uni[enc->byte_row(i1)][enc->byte_col(i2)] = j;
        }
      }
   read_done: ;
  }
}

static void do_ksc5601 (const char* name)
{
  Encoding enc;

  enc.rows = 94;
  enc.cols = 94;
  enc.row_byte = row_byte_normal;
  enc.col_byte = col_byte_normal;
  enc.byte_row = byte_row_normal;
  enc.byte_col = byte_col_normal;
  enc.check_row_expr = "%1$s >= 0x21 && %1$s < 0x7f";
  enc.check_col_expr = "%1$s >= 0x21 && %1$s < 0x7f";
  enc.byte_row_expr = "%1$s - 0x21";
  enc.byte_col_expr = "%1$s - 0x21";

  read_table_ksc5601(&enc);
  output_charset2uni(name,&enc);
  invert(&enc); output_uni2charset_sparse(name,&enc,false);
}

/* UHC specifics */

/* UHC part 1: 0x{81..A0}{41..5A,61..7A,81..FE} */

static int row_byte_uhc_1 (int row) {
  return 0x81 + row;
}
static int col_byte_uhc_1 (int col) {
  return (col >= 0x34 ? 0x4d : col >= 0x1a ? 0x47 : 0x41) + col;
}
static int byte_row_uhc_1 (int byte) {
  if (byte >= 0x81 && byte < 0xa1)
    return byte-0x81;
  else
    return -1;
}
static int byte_col_uhc_1 (int byte) {
  if (byte >= 0x41 && byte < 0x5b)
    return byte-0x41;
  else if (byte >= 0x61 && byte < 0x7b)
    return byte-0x47;
  else if (byte >= 0x81 && byte < 0xff)
    return byte-0x4d;
  else
    return -1;
}

static void do_uhc_1 (const char* name)
{
  Encoding enc;

  enc.rows = 32;
  enc.cols = 178;
  enc.row_byte = row_byte_uhc_1;
  enc.col_byte = col_byte_uhc_1;
  enc.byte_row = byte_row_uhc_1;
  enc.byte_col = byte_col_uhc_1;
  enc.check_row_expr = "(%1$s >= 0x81 && %1$s < 0xa1)";
  enc.check_col_expr = "(%1$s >= 0x41 && %1$s < 0x5b) || (%1$s >= 0x61 && %1$s < 0x7b) || (%1$s >= 0x81 && %1$s < 0xff)";
  enc.byte_row_expr = "%1$s - 0x81";
  enc.byte_col_expr = "%1$s - (%1$s >= 0x81 ? 0x4d : %1$s >= 0x61 ? 0x47 : 0x41)";

  read_table(&enc);
  output_charset2uni_noholes_monotonic(name,&enc);
  invert(&enc); output_uni2charset_sparse(name,&enc,true);
}

/* UHC part 2: 0x{A1..C6}{41..5A,61..7A,81..A0} */

static int row_byte_uhc_2 (int row) {
  return 0xa1 + row;
}
static int col_byte_uhc_2 (int col) {
  return (col >= 0x34 ? 0x4d : col >= 0x1a ? 0x47 : 0x41) + col;
}
static int byte_row_uhc_2 (int byte) {
  if (byte >= 0xa1 && byte < 0xff)
    return byte-0xa1;
  else
    return -1;
}
static int byte_col_uhc_2 (int byte) {
  if (byte >= 0x41 && byte < 0x5b)
    return byte-0x41;
  else if (byte >= 0x61 && byte < 0x7b)
    return byte-0x47;
  else if (byte >= 0x81 && byte < 0xa1)
    return byte-0x4d;
  else
    return -1;
}

static void do_uhc_2 (const char* name)
{
  Encoding enc;

  enc.rows = 94;
  enc.cols = 84;
  enc.row_byte = row_byte_uhc_2;
  enc.col_byte = col_byte_uhc_2;
  enc.byte_row = byte_row_uhc_2;
  enc.byte_col = byte_col_uhc_2;
  enc.check_row_expr = "(%1$s >= 0xa1 && %1$s < 0xff)";
  enc.check_col_expr = "(%1$s >= 0x41 && %1$s < 0x5b) || (%1$s >= 0x61 && %1$s < 0x7b) || (%1$s >= 0x81 && %1$s < 0xa1)";
  enc.byte_row_expr = "%1$s - 0xa1";
  enc.byte_col_expr = "%1$s - (%1$s >= 0x81 ? 0x4d : %1$s >= 0x61 ? 0x47 : 0x41)";

  read_table(&enc);
  output_charset2uni_noholes_monotonic(name,&enc);
  invert(&enc); output_uni2charset_sparse(name,&enc,true);
}

/* Big5 specifics */

static int row_byte_big5 (int row) {
  return 0xa1+row;
}
static int col_byte_big5 (int col) {
  return (col >= 0x3f ? 0x62 : 0x40) + col;
}
static int byte_row_big5 (int byte) {
  if (byte >= 0xa1 && byte < 0xff)
    return byte-0xa1;
  else
    return -1;
}
static int byte_col_big5 (int byte) {
  if (byte >= 0x40 && byte < 0x7f)
    return byte-0x40;
  else if (byte >= 0xa1 && byte < 0xff)
    return byte-0x62;
  else
    return -1;
}

static void do_big5 (const char* name)
{
  Encoding enc;

  enc.rows = 94;
  enc.cols = 157;
  enc.row_byte = row_byte_big5;
  enc.col_byte = col_byte_big5;
  enc.byte_row = byte_row_big5;
  enc.byte_col = byte_col_big5;
  enc.check_row_expr = "%1$s >= 0xa1 && %1$s < 0xff";
  enc.check_col_expr = "(%1$s >= 0x40 && %1$s < 0x7f) || (%1$s >= 0xa1 && %1$s < 0xff)";
  enc.byte_row_expr = "%1$s - 0xa1";
  enc.byte_col_expr = "%1$s - (%1$s >= 0xa1 ? 0x62 : 0x40)";

  read_table(&enc);
  output_charset2uni(name,&enc);
  invert(&enc); output_uni2charset_sparse(name,&enc,false);
}

/* HKSCS specifics */

static int row_byte_hkscs (int row) {
  return 0x80+row;
}
static int byte_row_hkscs (int byte) {
  if (byte >= 0x80 && byte < 0xff)
    return byte-0x80;
  else
    return -1;
}

static void do_hkscs (const char* name)
{
  Encoding enc;

  enc.rows = 128;
  enc.cols = 157;
  enc.row_byte = row_byte_hkscs;
  enc.col_byte = col_byte_big5;
  enc.byte_row = byte_row_hkscs;
  enc.byte_col = byte_col_big5;
  enc.check_row_expr = "%1$s >= 0x80 && %1$s < 0xff";
  enc.check_col_expr = "(%1$s >= 0x40 && %1$s < 0x7f) || (%1$s >= 0xa1 && %1$s < 0xff)";
  enc.byte_row_expr = "%1$s - 0x80";
  enc.byte_col_expr = "%1$s - (%1$s >= 0xa1 ? 0x62 : 0x40)";

  read_table(&enc);
  output_charset2uni(name,&enc);
  invert(&enc); output_uni2charset_sparse(name,&enc,false);
}

/* Johab Hangul specifics */

static int row_byte_johab_hangul (int row) {
  return 0x84+row;
}
static int col_byte_johab_hangul (int col) {
  return (col >= 0x3e ? 0x43 : 0x41) + col;
}
static int byte_row_johab_hangul (int byte) {
  if (byte >= 0x84 && byte < 0xd4)
    return byte-0x84;
  else
    return -1;
}
static int byte_col_johab_hangul (int byte) {
  if (byte >= 0x41 && byte < 0x7f)
    return byte-0x41;
  else if (byte >= 0x81 && byte < 0xff)
    return byte-0x43;
  else
    return -1;
}

static void do_johab_hangul (const char* name)
{
  Encoding enc;

  enc.rows = 80;
  enc.cols = 188;
  enc.row_byte = row_byte_johab_hangul;
  enc.col_byte = col_byte_johab_hangul;
  enc.byte_row = byte_row_johab_hangul;
  enc.byte_col = byte_col_johab_hangul;
  enc.check_row_expr = "%1$s >= 0x84 && %1$s < 0xd4";
  enc.check_col_expr = "(%1$s >= 0x41 && %1$s < 0x7f) || (%1$s >= 0x81 && %1$s < 0xff)";
  enc.byte_row_expr = "%1$s - 0x84";
  enc.byte_col_expr = "%1$s - (%1$s >= 0x81 ? 0x43 : 0x41)";

  read_table(&enc);
  output_charset2uni(name,&enc);
  invert(&enc); output_uni2charset_dense(name,&enc);
}

/* SJIS specifics */

static int row_byte_sjis (int row) {
  return (row >= 0x1f ? 0xc1 : 0x81) + row;
}
static int col_byte_sjis (int col) {
  return (col >= 0x3f ? 0x41 : 0x40) + col;
}
static int byte_row_sjis (int byte) {
  if (byte >= 0x81 && byte < 0xa0)
    return byte-0x81;
  else if (byte >= 0xe0)
    return byte-0xc1;
  else
    return -1;
}
static int byte_col_sjis (int byte) {
  if (byte >= 0x40 && byte < 0x7f)
    return byte-0x40;
  else if (byte >= 0x80 && byte < 0xfd)
    return byte-0x41;
  else
    return -1;
}

static void do_sjis (const char* name)
{
  Encoding enc;

  enc.rows = 94;
  enc.cols = 188;
  enc.row_byte = row_byte_sjis;
  enc.col_byte = col_byte_sjis;
  enc.byte_row = byte_row_sjis;
  enc.byte_col = byte_col_sjis;
  enc.check_row_expr = "(%1$s >= 0x81 && %1$s < 0xa0) || (%1$s >= 0xe0)";
  enc.check_col_expr = "(%1$s >= 0x40 && %1$s < 0x7f) || (%1$s >= 0x80 && %1$s < 0xfd)";
  enc.byte_row_expr = "%1$s - (%1$s >= 0xe0 ? 0xc1 : 0x81)";
  enc.byte_col_expr = "%1$s - (%1$s >= 0x80 ? 0x41 : 0x40)";

  read_table(&enc);
  output_charset2uni(name,&enc);
  invert(&enc); output_uni2charset_sparse(name,&enc,false);
}

/* GB18030 Unicode specifics */

static void do_gb18030uni (const char* name)
{
  int c;
  unsigned int bytes;
  int i1, i2, i3, i4, i, j, k;
  int charset2uni[4*10*126*10];
  int uni2charset[0x10000];
  struct { int low; int high; int diff; int total; } ranges[256];
  int ranges_count, ranges_total;

  for (i = 0; i < 4*10*126*10; i++)
    charset2uni[i] = 0;
  for (j = 0; j < 0x10000; j++)
    uni2charset[j] = 0;

  /* Read a unicode.org style .TXT file. */
  for (;;) {
    c = getc(stdin);
    if (c == EOF)
      break;
    if (c == '\n' || c == ' ' || c == '\t')
      continue;
    if (c == '#') {
      do { c = getc(stdin); } while (!(c == EOF || c == '\n'));
      continue;
    }
    ungetc(c,stdin);
    if (scanf("0x%x", &bytes) != 1)
      exit(1);
    i1 = (bytes >> 24) & 0xff;
    i2 = (bytes >> 16) & 0xff;
    i3 = (bytes >> 8) & 0xff;
    i4 = bytes & 0xff;
    if (!(i1 >= 0x81 && i1 <= 0x84
          && i2 >= 0x30 && i2 <= 0x39
          && i3 >= 0x81 && i3 <= 0xfe
          && i4 >= 0x30 && i4 <= 0x39)) {
      fprintf(stderr, "lost entry for %02x %02x %02x %02x\n", i1, i2, i3, i4);
      exit(1);
    }
    i = (((i1-0x81) * 10 + (i2-0x30)) * 126 + (i3-0x81)) * 10 + (i4-0x30);
    if (scanf(" 0x%x", &j) != 1)
      exit(1);
    if (!(j >= 0 && j < 0x10000))
      exit(1);
    charset2uni[i] = j;
    uni2charset[j] = i;
  }

  /* Verify that the mapping i -> j is monotonically increasing and
     of the form
        low[k] <= i <= high[k]  =>  j = diff[k] + i
     with a set of disjoint intervals (low[k], high[k]). */
  ranges_count = 0;
  for (i = 0; i < 4*10*126*10; i++)
    if (charset2uni[i] != 0) {
      int diff;
      j = charset2uni[i];
      diff = j - i;
      if (ranges_count > 0) {
        if (!(i > ranges[ranges_count-1].high))
          exit(1);
        if (!(j > ranges[ranges_count-1].high + ranges[ranges_count-1].diff))
          exit(1);
        /* Additional property: The diffs are also increasing. */
        if (!(diff >= ranges[ranges_count-1].diff))
          exit(1);
      }
      if (ranges_count > 0 && diff == ranges[ranges_count-1].diff)
        ranges[ranges_count-1].high = i;
      else {
        if (ranges_count == 256)
          exit(1);
        ranges[ranges_count].low = i;
        ranges[ranges_count].high = i;
        ranges[ranges_count].diff = diff;
        ranges_count++;
      }
    }

  /* Determine size of bitmap. */
  ranges_total = 0;
  for (k = 0; k < ranges_count; k++) {
    ranges[k].total = ranges_total;
    ranges_total += ranges[k].high - ranges[k].low + 1;
  }

  printf("static const unsigned short %s_charset2uni_ranges[%d] = {\n", name, 2*ranges_count);
  for (k = 0; k < ranges_count; k++) {
    printf("  0x%04x, 0x%04x", ranges[k].low, ranges[k].high);
    if (k+1 < ranges_count) printf(",");
    if ((k % 4) == 3 && k+1 < ranges_count) printf("\n");
  }
  printf("\n");
  printf("};\n");

  printf("\n");

  printf("static const unsigned short %s_uni2charset_ranges[%d] = {\n", name, 2*ranges_count);
  for (k = 0; k < ranges_count; k++) {
    printf("  0x%04x, 0x%04x", ranges[k].low + ranges[k].diff, ranges[k].high + ranges[k].diff);
    if (k+1 < ranges_count) printf(",");
    if ((k % 4) == 3 && k+1 < ranges_count) printf("\n");
  }
  printf("\n");
  printf("};\n");

  printf("\n");

  printf("static const struct { unsigned short diff; unsigned short bitmap_offset; } %s_ranges[%d] = {\n ", name, ranges_count);
  for (k = 0; k < ranges_count; k++) {
    printf(" { %5d, 0x%04x }", ranges[k].diff, ranges[k].total);
    if (k+1 < ranges_count) printf(",");
    if ((k % 4) == 3 && k+1 < ranges_count) printf("\n ");
  }
  printf("\n");
  printf("};\n");

  printf("\n");

  printf("static const unsigned char %s_bitmap[%d] = {\n ", name, (ranges_total + 7) / 8);
  {
    int accu = 0;
    for (k = 0; k < ranges_count; k++) {
      for (i = ranges[k].total; i <= ranges[k].total + (ranges[k].high - ranges[k].low);) {
        if (charset2uni[i - ranges[k].total + ranges[k].low] != 0)
          accu |= (1 << (i % 8));
        i++;
        if ((i % 8) == 0) {
          printf(" 0x%02x", accu);
          if ((i / 8) < (ranges_total + 7) / 8) printf(",");
          if (((i / 8) % 12) == 0)
            printf("\n ");
          accu = 0;
        }
      }
      if (i != (k+1 < ranges_count ? ranges[k+1].total : ranges_total)) abort();
    }
    if ((ranges_total % 8) != 0)
      printf(" 0x%02x", accu);
    printf("\n");
  }
  printf("};\n");

  printf("\n");

  printf("static int\n");
  printf("%s_mbtowc (conv_t conv, ucs4_t *pwc, const unsigned char *s, int n)\n", name);
  printf("{\n");
  printf("  unsigned char c1 = s[0];\n");
  printf("  if (c1 >= 0x81 && c1 <= 0x84) {\n");
  printf("    if (n >= 2) {\n");
  printf("      unsigned char c2 = s[1];\n");
  printf("      if (c2 >= 0x30 && c2 <= 0x39) {\n");
  printf("        if (n >= 3) {\n");
  printf("          unsigned char c3 = s[2];\n");
  printf("          if (c3 >= 0x81 && c3 <= 0xfe) {\n");
  printf("            if (n >= 4) {\n");
  printf("              unsigned char c4 = s[3];\n");
  printf("              if (c4 >= 0x30 && c4 <= 0x39) {\n");
  printf("                unsigned int i = (((c1 - 0x81) * 10 + (c2 - 0x30)) * 126 + (c3 - 0x81)) * 10 + (c4 - 0x30);\n");
  printf("                if (i >= %d && i <= %d) {\n", ranges[0].low, ranges[ranges_count-1].high);
  printf("                  unsigned int k1 = 0;\n");
  printf("                  unsigned int k2 = %d;\n", ranges_count-1);
  printf("                  while (k1 < k2) {\n");
  printf("                    unsigned int k = (k1 + k2) / 2;\n");
  printf("                    if (i <= %s_charset2uni_ranges[2*k+1])\n", name);
  printf("                      k2 = k;\n");
  printf("                    else if (i >= %s_charset2uni_ranges[2*k+2])\n", name);
  printf("                      k1 = k + 1;\n");
  printf("                    else\n");
  printf("                      return RET_ILSEQ;\n");
  printf("                  }\n");
  printf("                  {\n");
  printf("                    unsigned int bitmap_index = i - %s_charset2uni_ranges[2*k1] + %s_ranges[k1].bitmap_offset;\n", name, name);
  printf("                    if ((%s_bitmap[bitmap_index >> 3] >> (bitmap_index & 7)) & 1) {\n", name);
  printf("                      unsigned int diff = %s_ranges[k1].diff;\n", name);
  printf("                      *pwc = (ucs4_t) (i + diff);\n");
  printf("                      return 4;\n");
  printf("                    }\n");
  printf("                  }\n");
  printf("                }\n");
  printf("              }\n");
  printf("              return RET_ILSEQ;\n");
  printf("            }\n");
  printf("            return RET_TOOFEW(0);\n");
  printf("          }\n");
  printf("          return RET_ILSEQ;\n");
  printf("        }\n");
  printf("        return RET_TOOFEW(0);\n");
  printf("      }\n");
  printf("      return RET_ILSEQ;\n");
  printf("    }\n");
  printf("    return RET_TOOFEW(0);\n");
  printf("  }\n");
  printf("  return RET_ILSEQ;\n");
  printf("}\n");

  printf("\n");

  printf("static int\n");
  printf("%s_wctomb (conv_t conv, unsigned char *r, ucs4_t wc, int n)\n", name);
  printf("{\n");
  printf("  if (n >= 4) {\n");
  printf("    unsigned int i = wc;\n");
  printf("    if (i >= 0x%04x && i <= 0x%04x) {\n", ranges[0].low + ranges[0].diff, ranges[ranges_count-1].high + ranges[ranges_count-1].diff);
  printf("      unsigned int k1 = 0;\n");
  printf("      unsigned int k2 = %d;\n", ranges_count-1);
  printf("      while (k1 < k2) {\n");
  printf("        unsigned int k = (k1 + k2) / 2;\n");
  printf("        if (i <= %s_uni2charset_ranges[2*k+1])\n", name);
  printf("          k2 = k;\n");
  printf("        else if (i >= %s_uni2charset_ranges[2*k+2])\n", name);
  printf("          k1 = k + 1;\n");
  printf("        else\n");
  printf("          return RET_ILUNI;\n");
  printf("      }\n");
  printf("      {\n");
  printf("        unsigned int bitmap_index = i - %s_uni2charset_ranges[2*k1] + %s_ranges[k1].bitmap_offset;\n", name, name);
  printf("        if ((%s_bitmap[bitmap_index >> 3] >> (bitmap_index & 7)) & 1) {\n", name);
  printf("          unsigned int diff = %s_ranges[k1].diff;\n", name);
  printf("          i -= diff;\n");
  printf("          r[3] = (i %% 10) + 0x30; i = i / 10;\n");
  printf("          r[2] = (i %% 126) + 0x81; i = i / 126;\n");
  printf("          r[1] = (i %% 10) + 0x30; i = i / 10;\n");
  printf("          r[0] = i + 0x81;\n");
  printf("          return 4;\n");
  printf("        }\n");
  printf("      }\n");
  printf("    }\n");
  printf("    return RET_ILUNI;\n");
  printf("  }\n");
  printf("  return RET_TOOSMALL;\n");
  printf("}\n");
}

/* JISX0213 specifics */

static void do_jisx0213 (const char* name)
{
  printf("#ifndef _JISX0213_H\n");
  printf("#define _JISX0213_H\n");
  printf("\n");
  printf("/* JISX0213 plane 1 (= ISO-IR-233) characters are in the range\n");
  printf("   0x{21..7E}{21..7E}.\n");
  printf("   JISX0213 plane 2 (= ISO-IR-229) characters are in the range\n");
  printf("   0x{21,23..25,28,2C..2F,6E..7E}{21..7E}.\n");
  printf("   Together this makes 120 rows of 94 characters.\n");
  printf("*/\n");
  printf("\n");
  {
#define row_convert(row) \
      ((row) >= 0x121 && (row) <= 0x17E ? row-289 : /* 0..93 */    \
       (row) == 0x221                   ? row-451 : /* 94 */       \
       (row) >= 0x223 && (row) <= 0x225 ? row-452 : /* 95..97 */   \
       (row) == 0x228                   ? row-454 : /* 98 */       \
       (row) >= 0x22C && (row) <= 0x22F ? row-457 : /* 99..102 */  \
       (row) >= 0x26E && (row) <= 0x27E ? row-519 : /* 103..119 */ \
       -1)
    unsigned int table[120][94];
    int pagemin[0x1100];
    int pagemax[0x1100];
    int pageidx[0x1100];
    unsigned int pagestart[0x1100];
    unsigned int pagestart_len = 0;
    {
      unsigned int rowc, colc;
      for (rowc = 0; rowc < 120; rowc++)
        for (colc = 0; colc < 94; colc++)
          table[rowc][colc] = 0;
    }
    {
      unsigned int page;
      for (page = 0; page < 0x1100; page++)
        pagemin[page] = -1;
      for (page = 0; page < 0x1100; page++)
        pagemax[page] = -1;
      for (page = 0; page < 0x1100; page++)
        pageidx[page] = -1;
    }
    printf("static const unsigned short jisx0213_to_ucs_combining[][2] = {\n");
    {
      int private_use = 0x0001;
      for (;;) {
        char line[30];
        unsigned int row, col;
        unsigned int ucs;
        memset(line,0,sizeof(line));
        if (scanf("%[^\n]\n",line) < 1)
          break;
        assert(line[0]=='0');
        assert(line[1]=='x');
        assert(isxdigit(line[2]));
        assert(isxdigit(line[3]));
        assert(isxdigit(line[4]));
        assert(isxdigit(line[5]));
        assert(isxdigit(line[6]));
        assert(line[7]=='\t');
        line[7] = '\0';
        col = strtoul(&line[5],NULL,16);
        line[5] = '\0';
        row = strtoul(&line[2],NULL,16);
        if (line[20] != '\0' && line[21] == '\0') {
          unsigned int u1, u2;
          assert(line[8]=='0');
          assert(line[9]=='x');
          assert(isxdigit(line[10]));
          assert(isxdigit(line[11]));
          assert(isxdigit(line[12]));
          assert(isxdigit(line[13]));
          assert(line[14]==' ');
          assert(line[15]=='0');
          assert(line[16]=='x');
          assert(isxdigit(line[17]));
          assert(isxdigit(line[18]));
          assert(isxdigit(line[19]));
          assert(isxdigit(line[20]));
          u2 = strtoul(&line[17],NULL,16);
          line[14] = '\0';
          u1 = strtoul(&line[10],NULL,16);
          printf("  { 0x%04x, 0x%04x },\n", u1, u2);
          ucs = private_use++;
        } else {
          assert(line[8]=='0');
          assert(line[9]=='x');
          assert(isxdigit(line[10]));
          assert(isxdigit(line[11]));
          assert(isxdigit(line[12]));
          assert(isxdigit(line[13]));
          ucs = strtoul(&line[10],NULL,16);
        }
        assert((unsigned int) row_convert(row) < 120);
        assert((unsigned int) (col-0x21) < 94);
        table[row_convert(row)][col-0x21] = ucs;
      }
    }
    printf("};\n");
    printf("\n");
    {
      unsigned int rowc, colc;
      for (rowc = 0; rowc < 120; rowc++) {
        for (colc = 0; colc < 94; colc++) {
          unsigned int value = table[rowc][colc];
          unsigned int page = value >> 8;
          unsigned int rest = value & 0xff;
          if (pagemin[page] < 0 || pagemin[page] > rest) pagemin[page] = rest;
          if (pagemax[page] < 0 || pagemax[page] < rest) pagemax[page] = rest;
        }
      }
    }
    {
      unsigned int index = 0;
      unsigned int i;
      for (i = 0; i < 0x1100; ) {
        if (pagemin[i] >= 0) {
          if (pagemin[i+1] >= 0 && pagemin[i] >= 0x80 && pagemax[i+1] < 0x80) {
            /* Combine two pages into a single one. */
            assert(pagestart_len < sizeof(pagestart)/sizeof(pagestart[0]));
            pagestart[pagestart_len++] = (i<<8)+0x80;
            pageidx[i] = index;
            pageidx[i+1] = index;
            index++;
            i += 2;
          } else {
            /* A single page. */
            assert(pagestart_len < sizeof(pagestart)/sizeof(pagestart[0]));
            pagestart[pagestart_len++] = i<<8;
            pageidx[i] = index;
            index++;
            i += 1;
          }
        } else
          i++;
      }
    }
    printf("static const unsigned short jisx0213_to_ucs_main[120 * 94] = {\n");
    {
      unsigned int row;
      for (row = 0; row < 0x300; row++) {
        unsigned int rowc = row_convert(row);
        if (rowc != (unsigned int) (-1)) {
          printf("  /* 0x%X21..0x%X7E */\n",row,row);
          {
            unsigned int count = 0;
            unsigned int colc;
            for (colc = 0; colc < 94; colc++) {
              if ((count % 8) == 0) printf(" ");
              {
                unsigned int value = table[rowc][colc];
                unsigned int page = value >> 8;
                unsigned int index = pageidx[page];
                assert(value-pagestart[index] < 0x100);
                printf(" 0x%04x,",(index<<8)|(value-pagestart[index]));
              }
              count++;
              if ((count % 8) == 0) printf("\n");
            }
          }
          printf("\n");
        }
      }
    }
    printf("};\n");
    printf("\n");
    printf("static const ucs4_t jisx0213_to_ucs_pagestart[] = {\n");
    {
      unsigned int count = 0;
      unsigned int i;
      for (i = 0; i < pagestart_len; i++) {
        char buf[10];
        if ((count % 8) == 0) printf(" ");
        printf(" ");
        sprintf(buf,"0x%04x",pagestart[i]);
        if (strlen(buf) < 7) printf("%*s",7-strlen(buf),"");
        printf("%s,",buf);
        count++;
        if ((count % 8) == 0) printf("\n");
      }
    }
    printf("\n");
    printf("};\n");
#undef row_convert
  }
  rewind(stdin);
  printf("\n");
  {
    int table[0x110000];
    bool pages[0x4400];
    int maxpage = -1;
    unsigned int combining_prefixes[100];
    unsigned int combining_prefixes_len = 0;
    {
      unsigned int i;
      for (i = 0; i < 0x110000; i++)
        table[i] = -1;
      for (i = 0; i < 0x4400; i++)
        pages[i] = false;
    }
    for (;;) {
      char line[30];
      unsigned int plane, row, col;
      memset(line,0,sizeof(line));
      if (scanf("%[^\n]\n",line) < 1)
        break;
      assert(line[0]=='0');
      assert(line[1]=='x');
      assert(isxdigit(line[2]));
      assert(isxdigit(line[3]));
      assert(isxdigit(line[4]));
      assert(isxdigit(line[5]));
      assert(isxdigit(line[6]));
      assert(line[7]=='\t');
      line[7] = '\0';
      col = strtoul(&line[5],NULL,16);
      line[5] = '\0';
      row = strtoul(&line[3],NULL,16);
      line[3] = '\0';
      plane = strtoul(&line[2],NULL,16) - 1;
      if (line[20] != '\0' && line[21] == '\0') {
        unsigned int u1, u2;
        assert(line[8]=='0');
        assert(line[9]=='x');
        assert(isxdigit(line[10]));
        assert(isxdigit(line[11]));
        assert(isxdigit(line[12]));
        assert(isxdigit(line[13]));
        assert(line[14]==' ');
        assert(line[15]=='0');
        assert(line[16]=='x');
        assert(isxdigit(line[17]));
        assert(isxdigit(line[18]));
        assert(isxdigit(line[19]));
        assert(isxdigit(line[20]));
        u2 = strtoul(&line[17],NULL,16);
        line[14] = '\0';
        u1 = strtoul(&line[10],NULL,16);
        assert(u2 == 0x02E5 || u2 == 0x02E9 || u2 == 0x0300 || u2 == 0x0301
               || u2 == 0x309A);
        assert(combining_prefixes_len < sizeof(combining_prefixes)/sizeof(combining_prefixes[0]));
        combining_prefixes[combining_prefixes_len++] = u1;
      } else {
        unsigned int ucs;
        assert(line[8]=='0');
        assert(line[9]=='x');
        assert(isxdigit(line[10]));
        assert(isxdigit(line[11]));
        assert(isxdigit(line[12]));
        assert(isxdigit(line[13]));
        ucs = strtoul(&line[10],NULL,16);
        /* Add an entry. */
        assert(plane <= 1);
        assert(row <= 0x7f);
        assert(col <= 0x7f);
        table[ucs] = (plane << 15) | (row << 8) | col;
        pages[ucs>>6] = true;
        if (maxpage < 0 || (ucs>>6) > maxpage) maxpage = ucs>>6;
      }
    }
    {
      unsigned int i;
      for (i = 0; i < combining_prefixes_len; i++) {
        unsigned int u1 = combining_prefixes[i];
        assert(table[u1] >= 0);
        table[u1] |= 0x0080;
      }
    }
    printf("static const short jisx0213_from_ucs_level1[%d] = {\n",maxpage+1);
    {
      unsigned int index = 0;
      unsigned int i;
      for (i = 0; i <= maxpage; i++) {
        if ((i % 8) == 0) printf(" ");
        if (pages[i]) {
          printf(" %3u,",index);
          index++;
        } else {
          printf(" %3d,",-1);
        }
        if (((i+1) % 8) == 0) printf("\n");
      }
    }
    printf("\n");
    printf("};\n");
    printf("\n");
    #if 0 /* Dense array */
    printf("static const unsigned short jisx0213_from_ucs_level2[] = {\n");
    {
      unsigned int i;
      for (i = 0; i <= maxpage; i++) {
        if (pages[i]) {
          printf("  /* 0x%04X */\n",i<<6);
          {
            unsigned int j;
            for (j = 0; j < 0x40; ) {
              unsigned int ucs = (i<<6)+j;
              int value = table[ucs];
              if (value < 0) value = 0;
              if ((j % 8) == 0) printf(" ");
              printf(" 0x%04x,",value);
              j++;
              if ((j % 8) == 0) printf("\n");
            }
          }
        }
      }
    }
    printf("};\n");
    #else /* Sparse array */
    {
      int summary_indx[0x11000];
      int summary_used[0x11000];
      unsigned int i, k, indx;
      printf("static const unsigned short jisx0213_from_ucs_level2_data[] = {\n");
      /* Fill summary_indx[] and summary_used[]. */
      indx = 0;
      for (i = 0, k = 0; i <= maxpage; i++) {
        if (pages[i]) {
          unsigned int j1, j2;
          unsigned int count = 0;
          printf("  /* 0x%04X */\n",i<<6);
          for (j1 = 0; j1 < 4; j1++) {
            summary_indx[4*k+j1] = indx;
            summary_used[4*k+j1] = 0;
            for (j2 = 0; j2 < 16; j2++) {
              unsigned int j = 16*j1+j2;
              unsigned int ucs = (i<<6)+j;
              int value = table[ucs];
              if (value < 0) value = 0;
              if (value > 0) {
                summary_used[4*k+j1] |= (1 << j2);
                if ((count % 8) == 0) printf(" ");
                printf(" 0x%04x,",value);
                count++;
                if ((count % 8) == 0) printf("\n");
                indx++;
              }
            }
          }
          if ((count % 8) > 0)
            printf("\n");
          k++;
        }
      }
      printf("};\n");
      printf("\n");
      printf("static const Summary16 jisx0213_from_ucs_level2_2indx[] = {\n");
      for (i = 0, k = 0; i <= maxpage; i++) {
        if (pages[i]) {
          unsigned int j1;
          printf("  /* 0x%04X */\n",i<<6);
          printf(" ");
          for (j1 = 0; j1 < 4; j1++) {
            printf(" { %4d, 0x%04x },", summary_indx[4*k+j1], summary_used[4*k+j1]);
          }
          printf("\n");
          k++;
        }
      }
      printf("};\n");
    }
    #endif
    printf("\n");
  }
  printf("#ifdef __GNUC__\n");
  printf("__inline\n");
  printf("#else\n");
  printf("#ifdef __cplusplus\n");
  printf("inline\n");
  printf("#endif\n");
  printf("#endif\n");
  printf("static ucs4_t jisx0213_to_ucs4 (unsigned int row, unsigned int col)\n");
  printf("{\n");
  printf("  ucs4_t val;\n");
  printf("\n");
  printf("  if (row >= 0x121 && row <= 0x17e)\n");
  printf("    row -= 289;\n");
  printf("  else if (row == 0x221)\n");
  printf("    row -= 451;\n");
  printf("  else if (row >= 0x223 && row <= 0x225)\n");
  printf("    row -= 452;\n");
  printf("  else if (row == 0x228)\n");
  printf("    row -= 454;\n");
  printf("  else if (row >= 0x22c && row <= 0x22f)\n");
  printf("    row -= 457;\n");
  printf("  else if (row >= 0x26e && row <= 0x27e)\n");
  printf("    row -= 519;\n");
  printf("  else\n");
  printf("    return 0x0000;\n");
  printf("\n");
  printf("  if (col >= 0x21 && col <= 0x7e)\n");
  printf("    col -= 0x21;\n");
  printf("  else\n");
  printf("    return 0x0000;\n");
  printf("\n");
  printf("  val = jisx0213_to_ucs_main[row * 94 + col];\n");
  printf("  val = jisx0213_to_ucs_pagestart[val >> 8] + (val & 0xff);\n");
  printf("  if (val == 0xfffd)\n");
  printf("    val = 0x0000;\n");
  printf("  return val;\n");
  printf("}\n");
  printf("\n");
  printf("#ifdef __GNUC__\n");
  printf("__inline\n");
  printf("#else\n");
  printf("#ifdef __cplusplus\n");
  printf("inline\n");
  printf("#endif\n");
  printf("#endif\n");
  printf("static unsigned short ucs4_to_jisx0213 (ucs4_t ucs)\n");
  printf("{\n");
  printf("  if (ucs < (sizeof(jisx0213_from_ucs_level1)/sizeof(jisx0213_from_ucs_level1[0])) << 6) {\n");
  printf("    int index1 = jisx0213_from_ucs_level1[ucs >> 6];\n");
  printf("    if (index1 >= 0)");
  #if 0 /* Dense array */
  printf("\n");
  printf("      return jisx0213_from_ucs_level2[(index1 << 6) + (ucs & 0x3f)];\n");
  #else /* Sparse array */
  printf(" {\n");
  printf("      const Summary16 *summary = &jisx0213_from_ucs_level2_2indx[((index1 << 6) + (ucs & 0x3f)) >> 4];\n");
  printf("      unsigned short used = summary->used;\n");
  printf("      unsigned int i = ucs & 0x0f;\n");
  printf("      if (used & ((unsigned short) 1 << i)) {\n");
  printf("        /* Keep in `used' only the bits 0..i-1. */\n");
  printf("        used &= ((unsigned short) 1 << i) - 1;\n");
  printf("        /* Add `summary->indx' and the number of bits set in `used'. */\n");
  printf("        used = (used & 0x5555) + ((used & 0xaaaa) >> 1);\n");
  printf("        used = (used & 0x3333) + ((used & 0xcccc) >> 2);\n");
  printf("        used = (used & 0x0f0f) + ((used & 0xf0f0) >> 4);\n");
  printf("        used = (used & 0x00ff) + (used >> 8);\n");
  printf("        return jisx0213_from_ucs_level2_data[summary->indx + used];\n");
  printf("      };\n");
  printf("    };\n");
  #endif
  printf("  }\n");
  printf("  return 0x0000;\n");
  printf("}\n");
  printf("\n");
  printf("#endif /* _JISX0213_H */\n");
}

/* Main program */

int main (int argc, char *argv[])
{
  const char* charsetname;
  const char* name;

  if (argc != 3)
    exit(1);
  charsetname = argv[1];
  name = argv[2];

  output_title(charsetname);

  if (!strcmp(name,"gb2312")
      || !strcmp(name,"isoir165ext") || !strcmp(name,"gb12345ext")
      || !strcmp(name,"jisx0208") || !strcmp(name,"jisx0212"))
    do_normal(name);
  else if (!strcmp(name,"cns11643_1") || !strcmp(name,"cns11643_2")
           || !strcmp(name,"cns11643_3") || !strcmp(name,"cns11643_4a")
           || !strcmp(name,"cns11643_4b") || !strcmp(name,"cns11643_5")
           || !strcmp(name,"cns11643_6") || !strcmp(name,"cns11643_7")
           || !strcmp(name,"cns11643_15"))
    do_normal_only_charset2uni(name);
  else if (!strcmp(name,"cns11643_inv"))
    do_cns11643_only_uni2charset(name);
  else if (!strcmp(name,"gbkext1"))
    do_gbk1_only_charset2uni(name);
  else if (!strcmp(name,"gbkext2"))
    do_gbk2_only_charset2uni(name);
  else if (!strcmp(name,"gbkext_inv"))
    do_gbk1_only_uni2charset(name);
  else if (!strcmp(name,"cp936ext") || !strcmp(name,"gb18030ext"))
    do_gbk1(name);
  else if (!strcmp(name,"ksc5601"))
    do_ksc5601(name);
  else if (!strcmp(name,"uhc_1"))
    do_uhc_1(name);
  else if (!strcmp(name,"uhc_2"))
    do_uhc_2(name);
  else if (!strcmp(name,"big5") || !strcmp(name,"cp950ext"))
    do_big5(name);
  else if (!strcmp(name,"hkscs1999") || !strcmp(name,"hkscs2001")
           || !strcmp(name,"hkscs2004"))
    do_hkscs(name);
  else if (!strcmp(name,"johab_hangul"))
    do_johab_hangul(name);
  else if (!strcmp(name,"cp932ext"))
    do_sjis(name);
  else if (!strcmp(name,"gb18030uni"))
    do_gb18030uni(name);
  else if (!strcmp(name,"jisx0213"))
    do_jisx0213(name);
  else
    exit(1);

  return 0;
}
