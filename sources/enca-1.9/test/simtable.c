/* @(#) $Id: simtable.c,v 1.11 2003/11/17 12:27:39 yeti Exp $ */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "enca.h"
/* To cheat... */
#include "internal.h"

int myargc;
char **myargv;

static void
prl(const EncaLanguageInfo *l, const char *hooks)
{
  double *m;
  size_t i, j;

  if (myargc > 1) {
    i = 1;
    while (i < myargc && strcmp(myargv[i], l->name))
      i++;
    if (i == myargc)
      return;
  }

  printf("\n==\x1b[1m%s\x1b[m==\n", l->name);
  m = enca_get_charset_similarity_matrix(l);
  for (i = 0; i < l->ncharsets; i++) {
    for (j = 0; j < l->ncharsets; j++) {
      double q = 1000.0*m[i*l->ncharsets + j];

      if (i == j)
        printf("\x1b[36m");
      else if (q > 500)
        printf("\x1b[1;31m");
      else if (q > 333)
        printf("\x1b[31m");
      else if (q > 200)
        printf("\x1b[34m");
      else if (q < 50)
        printf("\x1b[30m");
      printf("%4.0f ", q);
      printf("\x1b[m");
    }
    printf("   %s\n", l->csnames[i]);
  }
  printf("Hooks: \x1b[32m%s\x1b[m\n", hooks);
}

int
main(int argc, char *argv[])
{
  myargc = argc;
  myargv = argv;

  prl(&ENCA_LANGUAGE_BE, "macwin isokoi 855866");
  prl(&ENCA_LANGUAGE_BG, "1251mac");
  prl(&ENCA_LANGUAGE_CS, "isowin 852kam");
  prl(&ENCA_LANGUAGE_ET, "");
  prl(&ENCA_LANGUAGE_HR, "isowin");
  prl(&ENCA_LANGUAGE_HU, "isocork isowin[XXX]");
  prl(&ENCA_LANGUAGE_LT, "winbalt lat4balt iso13win[XXX]");
  prl(&ENCA_LANGUAGE_LV, "winbalt iso13win[XXX]");
  prl(&ENCA_LANGUAGE_PL, "isowin balt13");
  prl(&ENCA_LANGUAGE_RU, "macwin");
  prl(&ENCA_LANGUAGE_SK, "isowin 852kam");
  prl(&ENCA_LANGUAGE_SL, "");
  prl(&ENCA_LANGUAGE_UK, "macwin isokoi ibm1125");

  return 0;
}
