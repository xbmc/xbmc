/*
  make_hash.c v2003-01-24
  make encodings.c from encodings.dat

  Copyright (C) 2000-2003 David Necas (Yeti) <yeti@physics.muni.cz>

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as published
  by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>

#ifdef HAVE_STRING_H
#  include <string.h>
#else /* HAVE_STRING_H */
#  ifdef HAVE_STRINGS_H
#    include <strings.h>
#  endif /* HAVE_STRINGS_H */
#endif /* HAVE_STRING_H */

#ifdef HAVE_MEMORY_H
#  include <memory.h>
#endif /* HAVE_MEMORY_H */

#include <unistd.h>
#include <ctype.h>

/* PARR {{{ */
#ifdef __GNUC__
# define PVAR(f, v) fprintf(stderr, "%s:%u %s(): " \
                            #v " == %" #f "\n", __FILE__, __LINE__, __FUNCTION__, v)
# define PARR(f, v, n) ( { int _i; \
  fprintf(stderr, "%s:%u %s(): " #v " == { ", __FILE__, __LINE__, __FUNCTION__); \
  for (_i = 0; _i < n; _i++) fprintf(stderr, "%" #f ", ", (v)[_i]); \
  fputs("}\n", stderr); \
} )
#else /* __GNUC__ */
/* FIXME */
#endif /* __GNUC__ */
/* }}} */

#define LEN 4096

typedef struct {
  char *enca;
  char *rfc1345;
  char *cstocs;
  char *iconv;
  char *mime;
  int naliases;
  char **aliases;
  char *human;
  char *flags;
  char *nsurface;
} EncaCharsetRaw;

typedef struct {
  int enca;
  int rfc1345;
  int cstocs;
  int iconv;
  int mime;
  char *human;
  char *flags;
  char *nsurface;
} EncaCharsetFine;

static EncaCharsetRaw RawNULL = {
  NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL
};

static char*
fixspaces(char *line)
{
  char *p, *q;
  int qs = 0;

  for (p = line; isspace(*p); p++)
    ;
  for (q = line; *p != '\0'; p++) {
    if (isspace(*p)) {
      *q = ' ';
      qs = 1;
    }
    else {
      if (qs) q++;
      *q++ = *p;
      qs = 0;
    }
  }
  *q = '\0';

  return line;
}

static int
add_item(const char *line,
         const char *name,
         char **item)
{
  const int len = strlen(name);

  if (*item != NULL) return 0;
  if (strncmp(line, name, len) != 0) return 0;
  *item = fixspaces(strdup(line + len));

  return 1;
}

static char**
check_alias(char **aliases,
            int *n,
            char *string)
{
  int i;

  if (string == NULL || string[0] == '\0') return aliases;
  for (i = 0; i < *n; i++)
    if (strcmp(aliases[i], string) == 0) return aliases;
  (*n)++;
  aliases = (char**)realloc(aliases, (*n)*sizeof(char*));
  aliases[*n - 1] = strdup(string);

  return aliases;
}

static EncaCharsetRaw*
read_raw_charset_data(FILE *stream,
                      int *rsize)
{
  char *line;
  EncaCharsetRaw *r, *raw;
  int rs;
  char *gl;

  line = (char*)malloc(LEN);
  r = raw = (EncaCharsetRaw*)malloc(sizeof(EncaCharsetRaw));
  *r = RawNULL;
  rs = 1;
  while (1) {
    gl = fgets(line, LEN, stream);
    if (r->enca && r->rfc1345 && r->cstocs && r->human && r->iconv && r->mime
        && r->flags && r->nsurface && r->aliases) {
      if (r->enca[0] == '\0') {
        fprintf(stderr, "Enca's charset name #%d empty\n", (int)(r - raw + 1));
        exit(1);
      }
      if (r->rfc1345[0] == '\0') {
        fprintf(stderr, "RFC-1345 charset name #%d empty\n", (int)(r - raw + 1));
        exit(1);
      }
      if (r->iconv[0] == '\0') r->iconv = NULL;
      if (r->cstocs[0] == '\0') r->cstocs = NULL;
      if (r->mime[0] == '\0') r->mime = NULL;
      if (r->nsurface[0] == '\0') r->nsurface = strdup("0");
      r->aliases = check_alias(r->aliases, &r->naliases, r->enca);
      r->aliases = check_alias(r->aliases, &r->naliases, r->iconv);
      r->aliases = check_alias(r->aliases, &r->naliases, r->rfc1345);
      r->aliases = check_alias(r->aliases, &r->naliases, r->mime);
      r->aliases = check_alias(r->aliases, &r->naliases, r->cstocs);
      if (!gl) break;
      rs++;
      {
        int d = r - raw;
        raw = (EncaCharsetRaw*)realloc(raw, rs*sizeof(EncaCharsetRaw));
        r = raw + d + 1;
      }
      *r = RawNULL;
    }
    line[LEN-1] = '\0';
    fixspaces(line);
    if (line[0] == '\0' || line[0] == '#') continue;
    if (add_item(line, "enca:", &r->enca)) continue;
    if (add_item(line, "rfc:", &r->rfc1345)) continue;
    if (add_item(line, "iconv:", &r->iconv)) continue;
    if (add_item(line, "mime:", &r->mime)) continue;
    if (add_item(line, "cstocs:", &r->cstocs)) continue;
    if (add_item(line, "human:", &r->human)) continue;
    if (add_item(line, "flags:", &r->flags)) continue;
    if (add_item(line, "nsurface:", &r->nsurface)) continue;
    if (strncmp(line, "aliases:", 8) == 0 && !r->aliases) {
      int i;
      char *next, *l = fixspaces(line+8);
      r->naliases = 1;
      while ((l = strchr(l, ' ')) != NULL) {
        r->naliases++;
        l++;
      }
      r->aliases = (char**)malloc((r->naliases)*sizeof(char*));
      l = line+8;
      for (i = 0; i < r->naliases; i++) {
        next = strchr(l, ' ');
        if (next) *next = '\0';
        r->aliases[i] = strdup(l);
        l = next+1;
      }
      continue;
    }
    fprintf(stderr, "Unexpected `%s'\n", line);
    exit(1);
  }

  *rsize = rs;
  return raw;
}

static int
squeeze_compare(const char *x, const char *y)
{
  while (*x != '\0' || *y != '\0') {
    while (*x != '\0' && !isalnum(*x)) x++;
    while (*y != '\0' && !isalnum(*y)) y++;
    if (tolower(*x) != tolower(*y))
      return (int)tolower(*x) - (int)tolower(*y);
    if (*x != '\0') x++;
    if (*y != '\0') y++;
  }
  return 0;
}

static int
stable_compare(const void *p, const void *q)
{
  char *x = *(char**)p;
  char *y = *(char**)q;
  int i;

  i = squeeze_compare(x, y);
  /* to stabilize the sort */
  if (i == 0) return strcmp(x, y);
  return i;
}

static int
bin_search(char **alist, const int n, const char *s)
{
  int i1 = 0;
  int i2 = n-1;
  int i;

  i = stable_compare(&s, &alist[i1]);
  if (i < 0) {
    fprintf(stderr, "Out of search range: `%s'\n", s);
    exit(0);
  }
  if (i == 0) return i1;

  i = stable_compare(&s, &alist[i2]);
  if (i > 0) {
    fprintf(stderr, "Out of search range: `%s'\n", s);
    exit(0);
  }
  if (i == 0) return i2;

  while (i1+1 < i2) {
    int im = (i1 + i2)/2;
    i = stable_compare(&s, &alist[im]);
    if (i == 0) return im;
    if (i > 0) i1 = im; else i2 = im;
  }
  if (stable_compare(&s, &alist[i1+1]) == 0) return i1+1;

  fprintf(stderr, "Not found: `%s'\n", s);
  exit(0);
}

static char**
build_alias_list(EncaCharsetRaw *raw, const int ncs, int *total)
{
  char **alist;
  int nn, i, j, k;

  for (i = nn = 0; i < ncs; i++) nn += raw[i].naliases;
  alist = (char**)malloc(nn*sizeof(char*));
  for (i = j = 0; i < ncs; i++) {
    for (k = 0; k < raw[i].naliases; k++)
      alist[j++] = raw[i].aliases[k];
  }
  qsort(alist, nn, sizeof(char*), &stable_compare);
  for (i = 1; i < nn; ) {
    if (squeeze_compare(alist[i], alist[i-1]) == 0) {
      if (strcmp(alist[i], alist[i-1]) == 0) {
        fprintf(stderr, "Removing duplicate `%s'\n", alist[i]);
        memmove(alist+i-1, alist+i, (nn-i)*sizeof(char*));
        nn--;
      }
      else {
        fprintf(stderr, "Keeping equvialent `%s' and `%s'\n",
                alist[i], alist[i-1]);
        i++;
      }
    }
    else i++;
  }

  *total = nn;
  return alist;
}

static EncaCharsetFine*
refine_data(EncaCharsetRaw *raw, const int ncs, char **alist, const int nn)
{
  int i;
  EncaCharsetFine *fine;

  fine = (EncaCharsetFine*)malloc(ncs*sizeof(EncaCharsetFine));

  for (i = 0; i < ncs; i++) {
    fine[i].enca = bin_search(alist, nn, raw[i].enca);
    fine[i].rfc1345 = bin_search(alist, nn, raw[i].rfc1345);
    fine[i].iconv = raw[i].iconv ? bin_search(alist, nn, raw[i].iconv) : -1;
    fine[i].cstocs = raw[i].cstocs ? bin_search(alist, nn, raw[i].cstocs) : -1;
    fine[i].mime = raw[i].mime ? bin_search(alist, nn, raw[i].mime) : -1;
    fine[i].human = raw[i].human;
    fine[i].flags = raw[i].flags;
    fine[i].nsurface = raw[i].nsurface;
  }

  return fine;
}

static int*
create_index_list(EncaCharsetRaw *raw, const int ncs,
                  char **alist, const int nn)
{
  int i, k;
  int *ilist;

  ilist = (int*)malloc(nn*sizeof(int));

  for (i = 0; i < ncs; i++) {
    for (k = 0; k < raw[i].naliases; k++) {
      ilist[bin_search(alist, nn, raw[i].aliases[k])] = i;
    }
  }

  return ilist;
}

static void
print_fine_data(EncaCharsetFine *fine, const int ncs,
                int *ilist, char **alist, const int nn)
{
  int i;

  puts("/****  THIS IS A GENERATED FILE.  DO NOT TOUCH!  *****/");

  puts("/* THIS IS A GENERATED TABLE, see tools/make_hash.c. */");
  puts("static const EncaCharsetInfo CHARSET_INFO[] = {");
  for (i = 0; i < ncs; i++) {
    printf("  {\n"
           "     %d, %d, %d, %d, %d,\n"
           "     \"%s\",\n"
           "     %s,\n"
           "     %s\n"
           "  },\n",
           fine[i].enca,
           fine[i].rfc1345,
           fine[i].cstocs,
           fine[i].iconv,
           fine[i].mime,
           fine[i].human,
           fine[i].flags,
           fine[i].nsurface);
  }
  puts("};\n");

  puts("/* THIS IS A GENERATED TABLE, see tools/make_hash.c. */");
  puts("static const char *ALIAS_LIST[] = {");
  for (i = 0; i < nn; i++) printf("  \"%s\",\n", alist[i]);
  puts("};\n");

  puts("/* THIS IS A GENERATED TABLE, see tools/make_hash.c. */");
  puts("static const int INDEX_LIST[] = {");
  for (i = 0; i < nn; i++) {
    if (i%16 == 0) printf("  ");
    printf("%2d, ", ilist[i]);
    if (i%16 == 15 || i == nn-1) printf("\n");
  }
  puts("};\n");
}

int
main(void)
{
  EncaCharsetRaw *raw;
  EncaCharsetFine *fine;
  char **alist;
  int *ilist;
  int ncs, nn;

  raw = read_raw_charset_data(stdin, &ncs);
  alist = build_alias_list(raw, ncs, &nn);
  fine = refine_data(raw, ncs, alist, nn);
  ilist = create_index_list(raw, ncs, alist, nn);
  print_fine_data(fine, ncs, ilist, alist, nn);

  return 0;
}
