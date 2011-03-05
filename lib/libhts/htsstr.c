/*
 *  String helper functions
 *  Copyright (C) 2008 Andreas Öman
 *  Copyright (C) 2008 Mattias Wadman
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "htsstr.h"


static void htsstr_argsplit_add(char ***argv, int *argc, char *s);
static int htsstr_format0(const char *str, char *out, char **map);

static char *
mystrndup(const char *src, size_t len)
{
  char *r = malloc(len + 1);
  r[len] = 0;
  return memcpy(r, src, len);
}


char *
htsstr_unescape(char *str) {
  char *s;

  for(s = str; *s; s++) {
    if(*s != '\\')
      continue;

    if(*(s + 1) == 'b')
      *s = '\b';
    else if(*(s + 1) == 'f')
      *s = '\f';
    else if(*(s + 1) == 'n')
      *s = '\n';
    else if(*(s + 1) == 'r')
      *s = '\r';
    else if(*(s + 1) == 't')
      *s = '\t';
    else
      *s = *(s + 1);

    if(*(s + 1)) {
      /* shift string left, copies terminator too */
      memmove(s + 1, s + 2, strlen(s + 2) + 1);
    }
  } 

  return str;
}

static void
htsstr_argsplit_add(char ***argv, int *argc, char *s)
{
  *argv = realloc(*argv, sizeof((*argv)[0]) * (*argc + 1));
  (*argv)[(*argc)++] = s;
}

char **
htsstr_argsplit(const char *str) {
  int quote = 0;
  int inarg = 0;
  const char *start = NULL;
  const char *stop = NULL;
  const char *s;
  char **argv = NULL;
  int argc = 0;

  for(s = str; *s; s++) {
    if(start && stop) {
      htsstr_argsplit_add(&argv, &argc,
                          htsstr_unescape(mystrndup(start, stop - start)));
      start = stop = NULL;
    }
    
    if(inarg) {
      switch(*s) {
        case '\\':
          s++;
          break;
        case '"':
          if(quote) {
            inarg = 0;
            quote = 0;
            stop = s;
          }
          break;
        case ' ':
          if(quote)
            break;
          inarg = 0;
          stop = s;
          break;
        default:
          break;
      }
    } else {
      switch(*s) {
        case ' ':
          break;
        case '"':
          quote = 1;
          s++;
          /* fallthru */
        default:
          inarg = 1;
          start = s;
          stop = NULL;
          break;
      }
    }
  }

  if(start) {
    if(!stop)
      stop = str + strlen(str);
    htsstr_argsplit_add(&argv, &argc,
                        htsstr_unescape(mystrndup(start, stop - start)));
  }

  htsstr_argsplit_add(&argv, &argc, NULL);

  return argv;
}

void 
htsstr_argsplit_free(char **argv) {
  int i;

  for(i = 0; argv[i]; i++)
    free(argv[i]);
  
  free(argv);
}

static int
htsstr_format0(const char *str, char *out, char **map) {
  const char *s = str;
  char *f;
  int n = 0;

  while(*s) {
    switch(*s) {
      case '%':
        f = map[(unsigned char)*(s + 1)];
        if(*(s + 1) != '%' && f) {
          s += 2; /* skip %f * */
          if(out)
            strcpy(&out[n], f);
          n += strlen(f);
          break;
        }
        /* fallthru */
      default:
        if(out)
          out[n] = *s;
        s++;
        n++;
        break;
    }
  }

  if(out)
    out[n] = '\0';

  return n + 1; /* + \0 */
}

char *
htsstr_format(const char *str, char **map)
{
  char *s;
  
  s = malloc(htsstr_format0(str, NULL, map));
  htsstr_format0(str, s, map);

  return s;
}

