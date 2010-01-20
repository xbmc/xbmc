/*
 * @(#) $Id: iconvcap.c,v 1.10 2005/12/01 10:08:53 yeti Exp $
 * iconv capability checker by David Necas (Yeti).
 * This program is in the public domain.
 *
 * iconvcap has two modes of operation:
 *
 * 1. No command line argumets are given.
 * Iconvcap tries to find what charsets of interest iconv knows and under
 * what names.  It prints #defines directly includable to C source for
 * any successfully detected charset (and #defines to NULL for the others).
 * It also prints some info to stderr, which then goes to config.log.  It
 * returns success (0) iff following conditions are satified
 * -- iconv is able to convert ISO-8859-1 to some tested variant of Unicode
 *    (see below)---so it's usable at all, and
 * -- iconv is able convert at least two of the other charsets of interest to
 *    the same variant of Unicode (we then hope conversion in the opposite
 *    direction will work too)
 * Otherwise, failure (1) is returned and the output should be ignored, if
 * any.
 *
 * 2. A file name is given on command line.
 * Iconvcap reads given file (should contain the just generated #define list)
 * and chcecks if conversion from any to any encoding is possible.  If it is
 * OK it returns success (0), otherwise, it fails returning 1.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <iconv.h>
/* string.h or strings.h?  That's a question!
 * Don't use const, the compiler may not like it. */
int strncmp(char *s1, char *s2, size_t n);
char* strchr(char *s, int c);
char* strrchr(char *s, int c);
char* strncpy(char *dest, char *src, size_t n);

#define DPREFIX "ICONV_NAME_"

#define TEST_ENC_TO_UNICODE(x) \
  fprintf(stderr, "iconvcap: checking for %s -> Unicode... ", #x); \
  if (iconv_check(VARIANT_##x, unicode) == 0) { \
    printf("#define "DPREFIX"%s \"%s\"\n", #x, FROM); \
    ok++; \
  } else printf("#define "DPREFIX"%s NULL\n", #x);

/* ANY variant of unicode is good enough here
   particular surfaces are defined below */
char* VARIANT_UNICODE[] = {
  "UCS2", "UCS-2", "ISO10646/UCS2", "ISO-10646/UCS2", "ISO_10646/UCS2",
  "UNICODE", "ISO-10646", "ISO_10646", "ISO10646",
  "UCS4", "UCS-4", "ISO10646/UCS4", "ISO-10646/UCS4", "ISO_10646/UCS4",
  "UTF8", "UTF-8", "ISO10646/UTF8", "ISO-10646/UTF8", "ISO_10646/UTF8",
  "CSUCS2", "CSUCS4", NULL
};

char* VARIANT_ASCII[] = {
  "ASCII", "CSASCII", "US-ASCII", "ISO646-US", "ISO_646.IRV:1991", "CP367",
  "IBM367", "CP367", "CSPC367", NULL
};

char* VARIANT_ISO88591[] = {
  "ISO-8859-1", "ISO8859-1", "8859_1", "ISO_8859-1", "LATIN1", "ISOLATIN1",
  "CSLATIN1", "CSISOLATIN1", NULL
};

char* VARIANT_ISO88592[] = {
  "ISO-8859-2", "ISO8859-2", "8859_2", "ISO_8859-2", "LATIN2", "ISOLATIN2",
  "CSLATIN2", "CSISOLATIN2", "ISO-IR-101", NULL
};

char* VARIANT_ISO88594[] = {
  "ISO-8859-4", "ISO8859-4", "8859_4", "ISO_8859-4", "LATIN4", "ISOLATIN4",
  "CSLATIN4", "CSISOLATIN4", "ISO-IR-110", NULL
};

char* VARIANT_ISO88595[] = {
  "ISO-8859-5", "ISO8859-5", "8859_5", "ISO_8859-5", "ISOCYRILLIC",
  "CSISOCYRILLIC", "ISO-IR-144", NULL
};

char* VARIANT_ISO885913[] = {
  "ISO-8859-13", "ISO8859-13", "8859_13", "ISO_8859-13", "LATIN7", "ISOLATIN7",
  "CSLATIN7", "CSISOLATIN7", "ISO-IR-179A", "ISOBALTIC", "CSISOBALTIC",
  "CSISOLATINBALTIC", NULL
};

char* VARIANT_ISO885916[] = {
  "ISO-8859-16", "ISO8859-16", "8859_16", "ISO_8859-16", "ISO-IR-226",
  "LATIN10", "ISOLATIN10", "CSLATIN10", "CSISOLATIN10", NULL
};

char* VARIANT_BALTIC[] = {
  "BALTIC", "CSBALTIC", "ISO-IR-179", NULL
};

char* VARIANT_IBM852[] = {
  "IBM852", "CP852", "CP-852", "CP_852", "852", "IBM-852", "IBM_852",
  "PC852", "CSPC852", "CSPCP852", NULL
};

char* VARIANT_IBM855[] = {
  "IBM855", "CP855", "CP-855", "CP_855", "855", "IBM-855", "IBM_855",
  "PC855", "CSPC855", "CSPCP855", NULL
};

char* VARIANT_IBM775[] = {
  "IBM775", "CP775", "CP-775", "CP_775", "775", "IBM-775", "IBM_775",
  "PC775", "CSPC775", "CSPCP775", NULL
};

char* VARIANT_IBM866[] = {
  "IBM866", "CP866", "CP-866", "CP_866", "866", "IBM-866", "IBM_866",
  "PC866", "CSPC866", "CSPCP866", NULL
};

char* VARIANT_CP1125[] = {
  "CP1125", "1125", "CP-1125", "CP_1125", "MS1125", "MS-1125",
  "WINDOWS-1125", NULL
};

char* VARIANT_CP1250[] = {
  "CP1250", "1250", "CP-1250", "CP_1250", "MS-EE", "MS1250", "MS-1250",
  "WINDOWS-1250", NULL
};

char* VARIANT_CP1251[] = {
  "CP1251", "1251", "CP-1251", "CP_1251", "MS-CYRL", "MS1251", "MS-1251",
  "WINDOWS-1251", NULL
};

char* VARIANT_CP1257[] = {
  "CP1257", "1257", "CP-1257", "CP_1257", "MS-BALT", "MS1257", "MS-1257",
  "WINDOWS-1257", "WinBaltRim", NULL
};

char* VARIANT_MACCE[] = {
  "MACCE", "MAC-CE", "MAC_CE", "MACINTOSH-CE", "MACEE", "MAC-EE", "MAC_EE",
  "MACINTOSH-EE", NULL
};

char* VARIANT_MACCYR[] = {
  "MACCYR", "MAC-CYR", "MAC_CYR", "MACINTOSH-CYR", "MACCYRILLIC",
  "MAC-CYRILLIC", "MACINTOSH-CYRILLIC", NULL
};

char* VARIANT_KOI8CS2[] = {
  "KOI8-CS2", "KOI8CS2", "KOI8_CS2", "KOI-8_CS2", "KOI8CS", "KOI8_CS",
  "KOI8-CS", "KOI-8-CS", "KOI_8-CS", "CSKOI8CS2", NULL
};

char* VARIANT_KOI8R[] = {
  "KOI8-R", "KOI8_R", "KOI-8_R", "KOI8R", "KOI8_R", "CSKOI8R", NULL
};

char* VARIANT_KOI8U[] = {
  "KOI8-U", "KOI8_U", "KOI-8_U", "KOI8U", "KOI8_U", "CSKOI8U", NULL
};

char* VARIANT_KOI8UNI[] = {
  "KOI8-UNI", "KOI8_UNI", "KOI-8_UNI", "KOI8UNI", "KOI8_UNI", "CSKOI8UNI",
  NULL
};

char* VARIANT_ECMA113[] = {
  "ECMA-113", "ECMA-cyrillic", "ECMA-113:1986", "ISO-IR-111", NULL
};

char* VARIANT_KEYBCS2[] = {
  "KEYBCS2", "KEYBCS-2", "KAM", "KAMENICKY", "CP895", "895", "PC895",
  "csPC895", NULL
};

char* VARIANT_LATEX[] = {
  "TEX", "LATEX", "LTEX", NULL
};

char* VARIANT_UCS2[] = {
  "UCS-2", "UCS-2BE", "UCS2", "ISO10646/UCS2", "ISO-10646/UCS2",
  "ISO_10646/UCS2", "CSUCS2", NULL
};

char* VARIANT_UCS4[] = {
  "UCS-4", "UCS-4BE", "UCS4", "ISO10646/UCS4", "ISO-10646/UCS4",
  "ISO_10646/UCS4", "CSUCS4", NULL
};

char* VARIANT_UTF7[] = {
  "UTF-7", "UTF7", "ISO10646/UTF7", "ISO-10646/UTF7", "ISO_10646/UTF7",
  "UNICODE/UTF7", "CSUTF7", NULL
};

char* VARIANT_UTF8[] = {
  "UTF-8", "UTF8", "ISO10646/UTF8", "ISO-10646/UTF8", "ISO_10646/UTF8",
  "UNICODE/UTF8", "CSUTF8", NULL
};

char* VARIANT_CORK[] = {
  "CORK", "T1", NULL
};

char* VARIANT_GBK[] = {
	"GBK", "GB2312", "CP936", NULL
};

char* VARIANT_BIG5[] = {
	"BIG5", "CP950", NULL
};

char* VARIANT_HZ[] = {
  "HZ", "HZ-GB-2312", NULL
};

typedef struct S_EncList {
  char *enc;
  struct S_EncList *next;
} T_EncList, *P_EncList;

/* for the case we would be linked with braindead librecode */
char *program_name = "iconvcap";

char *FROM, *TO;

/* Local protoypes. */
static int iconv_check        (char **fromlist,
                               char **tolist);
static int iconv_check_one    (char *from,
                               char *to);
static int check_transitivity (char *fname);

/* main() */
int
main(int argc, char *argv[])
{
  int ok;
  char *unicode[] = { NULL, NULL };

  /* when we are called with some argument, run transitivity test and exit */
  if (argc > 1) return check_transitivity(argv[1]);

  /* check for conversion ISO-8859-1 -> Unicode */
  fprintf(stderr, "iconvcap: checking for ISO8859-1 -> Unicode... ");
  if ((ok = iconv_check(VARIANT_ISO88591, VARIANT_UNICODE)) == 0) {
    unicode[0] = TO;
    printf("#define "DPREFIX"UNICODE \"%s\"\n", unicode[0]);
  } else {
    fprintf(stderr, "iconvcap: iconv seems to be broken. aborting.\n");
    exit(1);
  }

  /* create table of charset names how iconv uses them */
  ok = 0;
  TEST_ENC_TO_UNICODE(ASCII);
  TEST_ENC_TO_UNICODE(BALTIC);
  TEST_ENC_TO_UNICODE(CP1125);
  TEST_ENC_TO_UNICODE(CP1250);
  TEST_ENC_TO_UNICODE(CP1251);
  TEST_ENC_TO_UNICODE(CP1257);
  TEST_ENC_TO_UNICODE(ECMA113);
  TEST_ENC_TO_UNICODE(IBM852);
  TEST_ENC_TO_UNICODE(IBM855);
  TEST_ENC_TO_UNICODE(IBM775);
  TEST_ENC_TO_UNICODE(IBM866);
  TEST_ENC_TO_UNICODE(ISO88592);
  TEST_ENC_TO_UNICODE(ISO88594);
  TEST_ENC_TO_UNICODE(ISO88595);
  TEST_ENC_TO_UNICODE(ISO885913);
  TEST_ENC_TO_UNICODE(ISO885916);
  TEST_ENC_TO_UNICODE(KEYBCS2);
  TEST_ENC_TO_UNICODE(KOI8CS2);
  TEST_ENC_TO_UNICODE(KOI8R);
  TEST_ENC_TO_UNICODE(KOI8U);
  TEST_ENC_TO_UNICODE(KOI8UNI);
  TEST_ENC_TO_UNICODE(MACCE);
  TEST_ENC_TO_UNICODE(MACCYR);
  TEST_ENC_TO_UNICODE(LATEX);
  TEST_ENC_TO_UNICODE(UCS2);
  TEST_ENC_TO_UNICODE(UCS4);
  TEST_ENC_TO_UNICODE(UTF7);
  TEST_ENC_TO_UNICODE(UTF8);
  TEST_ENC_TO_UNICODE(CORK);
  TEST_ENC_TO_UNICODE(GBK);
  TEST_ENC_TO_UNICODE(BIG5);
  TEST_ENC_TO_UNICODE(HZ);
  
  if (ok >= 2) exit(0);
  else exit(1);
}

/* return 0 if conversion from any charset from fromlist to any charset from
   tolist is possible and set FROM and TO (globals) to appropriate names
   (it's assumed fromlist and tolist are lists of charset aliases) */
static int
iconv_check(char **fromlist, char **tolist)
{
  char **from, **to;

  for (from = fromlist; *from != NULL; from++) {
    for (to = tolist; *to != NULL; to++) {
      if (iconv_check_one(*from, *to) == 0) {
        fprintf(stderr, "found %s -> %s\n", *from, *to);
        FROM = *from;
        TO = *to;
        return 0;
      }
    }
  }
  fprintf(stderr, "failed.\n");
  FROM = NULL;
  TO = NULL;
  return 1;
}

/* check if conversion from any encoding not defined as NULL in file fname
   to any other defined there is possible, in other words check transitivity
   condition for all defined encodings (we then hope this condition holds
   also for encodings we don't know anything about)
   returns 0 on success 1 on failure */
static int
check_transitivity(char *fname)
{
  char *s, *sb, *se;
  FILE *f;
  P_EncList enclist = NULL;
  P_EncList p_e;


  s = (char*)malloc(1024);
  if ((f = fopen(fname, "r")) == NULL) {
    fprintf(stderr, "iconvcap: cannot open %s\n", fname);
    free(s);
    return 1;
  }

  while (fgets(s, 1024, f) != NULL) {
    p_e = (P_EncList)malloc(sizeof(T_EncList));
    if (strncmp(s, "#define", 7) != 0) {
      fprintf(stderr, "iconvcap: malformed input line: %s", s);
      fclose(f);
      free(s);
      return 1;
    }
    if ((sb = strchr(s, '"')) != NULL) {
      if ((se = strrchr(s, '"')) == sb) {
        fprintf(stderr, "iconvcap: malformed input line: %s", s);
        fclose(f);
        free(s);
        return 1;
      }

      p_e->enc = strncpy((char*)malloc(se-sb), sb+1, se-sb-1);
      p_e->enc[se-sb-1] = '\0';
      p_e->next = enclist;
      enclist = p_e;
    }
  }
  fclose(f);

  if (enclist == NULL) {
    fprintf(stderr, "no valid encodings\n");
    free(s);
    return 1;
  }

  while (enclist != NULL) {
    for (p_e = enclist->next; p_e != NULL; p_e = p_e->next) {
      if (iconv_check_one(enclist->enc, p_e->enc) != 0) {
        fprintf(stderr, "iconvap: iconv_open(%s, %s) failed\n",
                enclist->enc, p_e->enc);
        free(s);
        return 1;
      }
      if (iconv_check_one(p_e->enc, enclist->enc) != 0) {
        fprintf(stderr, "iconvcap: iconv_open(%s, %s) failed\n",
                p_e->enc, enclist->enc);
        free(s);
        return 1;
      }
    }
    enclist = enclist->next;
  }

  fprintf(stderr, "iconvcap: transitivity OK\n");
  free(s);
  return 0;
}

/* check whether conversion from `from' to `to' is possible */
static int
iconv_check_one(char *from, char *to)
{
  iconv_t id;

  id = iconv_open(from, to);
  if (id == (iconv_t)(-1)) return 1;
  iconv_close(id);
  return 0;
}

