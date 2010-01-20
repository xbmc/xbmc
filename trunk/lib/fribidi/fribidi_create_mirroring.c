/* FriBidi - Library of BiDi algorithm
 * Copyright (C) 2001,2002 Behdad Esfahbod. 
 * 
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this library, in a file named COPYING; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA  
 * 
 * For licensing issues, contact <fwpg@sharif.edu>. 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "fribidi_unicode.h"

static void
err2 (const char *fmt, char *p)
{
  fprintf (stderr, "fribidi_create_mirroring: error: ");
  fprintf (stderr, fmt, p);
  fprintf (stderr, "\n");
  exit (1);
}

static int table[0x110000];
static char *bidi_mirroring_file;

static int mirroring_count;

static void
read_bidi_mirroring (void)
{
  char s[500];
  unsigned int i, j;
  FILE *f;

  for (i = 0; i < 0x110000; i++)
    table[i] = 0;
  mirroring_count = 0;
  printf ("Reading `BidiMirroring.txt'\n");
  if (!(f = fopen (bidi_mirroring_file, "rt")))
    err2 ("cannot open `%s' for reading", bidi_mirroring_file);
/*  fgets (s, sizeof s, f);
  sscanf (s, "# BidiMirroring-%s.txt", bidi_mirroring_version = malloc (20));*/
  while (fgets (s, sizeof s, f))
    {
      if (s[0] == '#' || s[0] == '\0' || s[0] == '\n')
	continue;
      sscanf (s, "%x; %x", &i, &j);
      table[i] = j;
      table[j] = i;
      mirroring_count++;
    }
  fclose (f);
}

static char *
headermacro (char *file)
{
  char *t = strdup (file);
  char *p = t;
  while (*p)
    {
      if (*p >= 'a' && *p <= 'z')
	*p += 'A' - 'a';
      else if ((*p < 'A' || *p > 'Z') && (*p < '0' || *p > '9'))
	*p = '_';
      p++;
    }
  return t;
}

static void
write_mirror (char *file)
{
  int i;
  FILE *f;
  char *FILENAME = headermacro (file);

  printf ("Writing `%s'\n", file);
  if (!(f = fopen (file, "wt")))
    err2 ("cannot open `%s' for writing", file);
  fprintf (f, "/*\n"
	   "  This file was automatically created from BidiMirroring.txt, version %s\n"
	   "  by fribidi_create_mirroring\n*/\n\n", FRIBIDI_UNICODE_VERSION);
  fprintf (f, "#ifndef %s\n#define %s\n\n#include \"fribidi.h\"\n\n",
	   FILENAME, FILENAME);
  fprintf (f, "/*\n"
	   "  Mirrored characters include all the characters in the Unicode list\n"
	   "  that have been declared as being mirrored and that have a mirrored\n"
	   "  equivalent.\n"
	   "\n"
	   "  There are lots of characters that are designed as being mirrored\n"
	   "  but do not have any mirrored glyph, e.g. the sign for there exist.\n");
  fprintf (f,
	   "  Are these used in Arabic? That is are all the mathematical signs\n"
	   "  that are assigned to be mirrorable actually mirrored in Arabic?\n"
	   "  If that is the case, we'll change the below code to include also\n"
	   "  characters that mirror to themself. It will then be the responsibility\n"
	   "  of the display engine to actually mirror these.\n" "*/\n\n");
  fprintf (f, "/* *INDENT-OFF" "* */\n\n");
  fprintf (f, "static const struct\n"
	   "{\n"
	   "  FriBidiChar ch, mirrored_ch;\n"
	   "}\n" "FriBidiMirroredChars[] =\n" "{\n");
  for (i = 0; i < 0x110000; i++)
    if (table[i])
      fprintf (f, "  {0x%04X, 0x%04X},\n", i, table[i]);
  fprintf (f, "} ;\n\n");
  fprintf (f, "/* *INDE" "NT-ON* */\n\n");
  fprintf (f, "const int nFriBidiMirroredChars = %d;\n\n", mirroring_count);
  fprintf (f, "\n#endif /* %s */\n", FILENAME);
  fclose (f);
}

int
main (int argc, char **argv)
{
  const char *p;

  p = (argc >= 2) ? argv[1] : "unidata";
  bidi_mirroring_file = malloc (50 + strlen (p));
  sprintf (bidi_mirroring_file, "%s/BidiMirroring.txt", p);
  read_bidi_mirroring ();
  write_mirror ("fribidi_tab_mirroring.i");
  return 0;
}
