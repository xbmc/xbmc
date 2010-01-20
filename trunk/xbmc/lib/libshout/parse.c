/* parse.c
 * metadata parsing routines
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "srconfig.h"
#include "mchar.h"
#include "debug.h"
#include "srtypes.h"
#include "regex.h"

/* GCS FIX: figure out function prototypes for regwcomp, regwexec */

/*****************************************************************************
 * Private global variables
 *****************************************************************************/
#define MAX_RULE_SIZE                2048
#define MAX_SUBMATCHES                 24

#define PARSERULE_CMD_MATCH          0x01
#define PARSERULE_CMD_SUBST          0x02

#define PARSERULE_SKIP               0x01
#define PARSERULE_GLOBAL             0x02
#define PARSERULE_ICASE              0x04
#define PARSERULE_SAVE               0x08
#define PARSERULE_EXCLUDE            0x10


struct parse_rule {
    int cmd;
    int flags;
    int artist_idx;
    int title_idx;
    int album_idx;
    int trackno_idx;
    regex_t* reg;
    mchar* match;
    mchar* subst;
};
typedef struct parse_rule Parse_Rule;

static Parse_Rule m_default_rule_list[] = {
    { PARSERULE_CMD_MATCH, 
      PARSERULE_SKIP,
      0, 0, 0, 0,
      0, 
#if defined HAVE_WCHAR_SUPPORT
      /* GCS FIX: L string literals are not good */
      L"^A suivre:",
      L""
#else
      "^A suivre:",
      ""
#endif
    },
    { PARSERULE_CMD_SUBST, 
      PARSERULE_ICASE,
      0, 0, 0, 0,
      0, 
#if defined HAVE_WCHAR_SUPPORT
      L"[[:space:]]*-?[[:space:]]*mp3pro$",
      L""
#else
      "[[:space:]]*-?[[:space:]]*mp3pro$",
      ""
#endif
    },
    { PARSERULE_CMD_MATCH, 
      PARSERULE_ICASE,
      1, 2, 0, 0,
      0, 
#if defined HAVE_WCHAR_SUPPORT
      L"^[[:space:]]*([^-]*[^-[:space:]])[[:space:]]*-[[:space:]]*(.*)[[:space:]]*$",
      L""
#else
      "^[[:space:]]*([^-]*[^-[:space:]])[[:space:]]*-[[:space:]]*(.*)[[:space:]]*$",
      ""
#endif
    },
    { 0x00, 
      0x00,
      0, 0, 0, 0,
      0, 
#if defined HAVE_WCHAR_SUPPORT
      L"",
      L""
#else
      "",
      ""
#endif
    }
};
static Parse_Rule* m_global_rule_list = m_default_rule_list;

/*****************************************************************************
 * Public functions
 *****************************************************************************/
static int
sr_regcomp (Parse_Rule* pr, mchar* rule_string, int cflags)
{
#if defined HAVE_WCHAR_SUPPORT
    return regwcomp(pr->reg, rule_string, cflags);
#else
    return regcomp(pr->reg, rule_string, cflags);
#endif
}

static int
mregexec (const regex_t* preg, const mchar* string, size_t nmatch,
	  regmatch_t pmatch[], int eflags)
{
#if defined HAVE_WCHAR_SUPPORT
    return regwexec (preg, string, nmatch, pmatch, eflags);
#else
    return regexec (preg, string, nmatch, pmatch, eflags);
#endif
}

/* Returns 1 if successful, 0 if failure */
static int
compile_rule (Parse_Rule* pr, mchar* rule_string)
{
    int rc;
    int cflags;

    pr->reg = (regex_t*) malloc (sizeof(regex_t));
    if (!pr->reg) return 0;

    cflags = REG_EXTENDED;
    if (pr->flags & PARSERULE_ICASE) {
	cflags |= REG_ICASE;
    }
    rc = sr_regcomp(pr, rule_string, cflags);
    if (rc != 0) {
	free(pr->reg);
	return 0;
    }
    return 1;
}

static void
use_default_rules (void)
{
    Parse_Rule* rulep;

    /* set global rule list to default */
    m_global_rule_list = m_default_rule_list;

    /* compile regular expressions */
    for (rulep = m_global_rule_list; rulep->cmd; rulep++) {
	compile_rule (rulep, rulep->match);
    }
}

static void
copy_rule_result (mchar* dest, mchar* query_string, 
		  regmatch_t* pmatch, int idx)
{
    if (idx > 0 && idx <= MAX_SUBMATCHES) {
	mstrncpy (dest, query_string + pmatch[idx].rm_so,
		  pmatch[idx].rm_eo - pmatch[idx].rm_so + 1);
    }
}

/* Return 1 if successful, or 0 for failure */
static int
parse_flags (Parse_Rule* pr, char* flags)
{
    char flag1;

    while ((flag1 = *flags++)) {
        int* tag = 0;
	switch (flag1) {
	case 'e':
	    pr->flags |= PARSERULE_SKIP;
	    if (pr->cmd != PARSERULE_CMD_MATCH) return 0;
	    break;
	case 'g':
	    /* GCS FIX: Not yet implemented */
	    pr->flags |= PARSERULE_GLOBAL;
	    break;
	case 'i':
	    pr->flags |= PARSERULE_ICASE;
	    break;
	case 's':
	    pr->flags |= PARSERULE_SAVE;
	    if (pr->cmd != PARSERULE_CMD_MATCH) return 0;
	    break;
	case 'x':
	    pr->flags |= PARSERULE_EXCLUDE;
	    if (pr->cmd != PARSERULE_CMD_MATCH) return 0;
	    break;
	case 'A':
	    tag = &pr->artist_idx;
	    break;
	case 'C':
	    tag = &pr->album_idx;
	    break;
	case 'N':
	    tag = &pr->trackno_idx;
	    break;
	case 'T':
	    tag = &pr->title_idx;
	    break;
	case 0:
	case '\n':
	case '\r':
	    return 1;
	default:
	    return 0;
	}
	if (tag) {
	    int rc;
	    int nchar;
	    int idx = 0;

	    if (pr->cmd != PARSERULE_CMD_MATCH) return 0;
	    rc = sscanf (flags, "%d%n", &idx, &nchar);
	    if (rc == 0 || idx > MAX_SUBMATCHES) {
		return 0;
	    }
	    flags += nchar;
	    *tag = idx;
	}
    }
    return 1;
}

static char* 
parse_escaped_string (char* outbuf, char* inbuf)
{
    int escaped = 0;

    while (1) {
	switch (*outbuf++ = *inbuf++) {
	case '\\':
	    escaped = !escaped;
	    break;
	case '/':
	    if (!escaped){
		*(--outbuf) = 0;
		return inbuf;
	    }
	    break;
	case 0:
	    return 0;
	    break;
	default:
	    escaped = 0;
	    break;
	}
    }
    /* Never get here */
    return 0;
}

/* This mega-function reads in the rules file, and loads 
    all the rules into the m_global_rules_list data structure */
void
init_metadata_parser (char* rules_file)
{
    FILE* fp;
    int ri;     /* Rule index */

    if (!rules_file || !*rules_file) {
	use_default_rules ();
	return;
    }
    fp = fopen (rules_file, "r");
    if (!fp) {
	use_default_rules ();
	return;
    }

    m_global_rule_list = 0;
    ri = 0;
    while (!feof(fp)) {
	char rule_buf[MAX_RULE_SIZE];
	char match_buf[MAX_RULE_SIZE];
	char subst_buf[MAX_RULE_SIZE];
	mchar w_match_buf[MAX_RULE_SIZE];
	mchar w_subst_buf[MAX_RULE_SIZE];
	char* rbp;
	int got_command;
	int rc;
	
	/* Allocate memory for rule */
	m_global_rule_list = realloc (m_global_rule_list,
				      (ri+1) * sizeof(Parse_Rule));
	memset (&m_global_rule_list[ri], 0, sizeof(Parse_Rule));

	/* Get next line from file */
	fgets (rule_buf,2048,fp);

	/* Skip leading whitespace */
	rbp = rule_buf;
	while (*rbp && isspace(*rbp)) rbp++;
	if (!*rbp) continue;

	/* Get command */
	got_command = 0;
	switch (*rbp++) {
	case 'm':
	    got_command = 1;
	    m_global_rule_list[ri].cmd = PARSERULE_CMD_MATCH;
	    break;
	case 's':
	    got_command = 1;
	    m_global_rule_list[ri].cmd = PARSERULE_CMD_SUBST;
	    break;
	case '#':
	    got_command = 0;
	    break;
	default:
	    got_command = 0;
	    printf ("Warning: malformed command in rules file:\n%s\n",
		    rule_buf);
	    break;
	}
	if (!got_command) continue;

	/* Skip past fwd slash */
	if (*rbp++ != '/') {
	    printf ("Warning: malformed command in rules file:\n%s\n",
		    rule_buf);
	    continue;
	}

	/* Parse match string */
	rbp = parse_escaped_string (match_buf, rbp);
	debug_printf ("match_buf=%s\n", match_buf);
	if (!rbp) {
	    printf ("Warning: malformed command in rules file:\n%s\n",
		    rule_buf);
	    continue;
	}

	/* Parse subst string */
	if (m_global_rule_list[ri].cmd == PARSERULE_CMD_SUBST) {
	    rbp = parse_escaped_string (subst_buf, rbp);
	    debug_printf ("subst_buf=%s\n", subst_buf);
	    if (!rbp) {
		printf ("Warning: malformed command in rules file:\n%s\n",
			rule_buf);
		continue;
	    }
	}

	/* Parse flags */
	rc = parse_flags (&m_global_rule_list[ri], rbp);
	if (!rc) {
	    printf ("Warning: malformed command in rules file:\n%s\n",
		    rule_buf);
	    continue;
	}

	/* Compile the rule */
	debug_printf ("Compiling the rule\n");
	mstring_from_string (w_match_buf, MAX_RULE_SIZE, match_buf, 
			     CODESET_UTF8);
	if (!compile_rule(&m_global_rule_list[ri], w_match_buf)) {
	    printf ("Warning: malformed regular expression:\n%s\n", 
		    match_buf);
	    continue;
	}

	/* Copy rule strings */
	debug_printf ("Copying rule string (1)\n");
	debug_mprintf (m_("String is ") m_S m_("\n"), w_match_buf);
	m_global_rule_list[ri].match = mstrdup(w_match_buf);
	debug_printf ("Copying rule string (2)\n");
	if (m_global_rule_list[ri].cmd == PARSERULE_CMD_SUBST) {
	    debug_printf ("Copying rule string (3)\n");
	    mstring_from_string (w_subst_buf, MAX_RULE_SIZE, subst_buf, 
				 CODESET_UTF8);
	    debug_printf ("Copying rule string (4)\n");
	    m_global_rule_list[ri].subst = mstrdup(w_subst_buf);
	    debug_printf ("Copying rule string (5)\n");
	}

	debug_printf ("End of loop\n");
	ri++;
    }
    /* Add null rule to the end */
    m_global_rule_list = realloc (m_global_rule_list,
				  (ri+1) * sizeof(Parse_Rule));
    memset (&m_global_rule_list[ri], 0, sizeof(Parse_Rule));
}

void
compose_metadata (TRACK_INFO* ti)
{
    int num_bytes;
    unsigned char num_16_bytes;
    mchar w_composed_metadata[MAX_METADATA_LEN+1];

    if (ti->have_track_info) {
	if (ti->artist[0]) {
	    msnprintf (w_composed_metadata, MAX_METADATA_LEN,
		       m_("StreamTitle='") m_S m_(" - ") m_S m_("';"),
		       ti->artist, ti->title);
	} else {
	    msnprintf (w_composed_metadata, MAX_METADATA_LEN,
		       m_("StreamTitle='") m_S m_("';"),
		       ti->title);
	}
    } else {
	debug_printf ("No track info when composing relay metadata\n");
    }
    debug_printf ("Converting relay string to char\n");
    num_bytes = string_from_mstring (&ti->composed_metadata[1], 
				     MAX_METADATA_LEN, 
				     w_composed_metadata, 
				     CODESET_RELAY);
    ti->composed_metadata[MAX_METADATA_LEN] = 0;  // note, not LEN-1
    num_16_bytes = (num_bytes + 15) / 16;
    ti->composed_metadata[0] = num_16_bytes;
}

void
parse_metadata (TRACK_INFO* ti)
{
    int eflags;
    int rc;
    mchar query_string[MAX_TRACK_LEN];
    Parse_Rule* rulep;

    /* Has any m/.../s rule matched? */
    BOOL save_track_matched = FALSE;

    /* Has any m/.../x rule matched? */
    BOOL exclude_track_matched = FALSE;

    ti->have_track_info = 0;
    ti->artist[0] = 0;
    ti->title[0] = 0;
    ti->album[0] = 0;
    ti->composed_metadata[0] = 0;
    ti->save_track = TRUE;
    if (!ti->raw_metadata[0]) {
	debug_printf ("Couldn't parse because no meta data\n");
	return;
    }

    /* Loop through rules, if we find a matching rule, then use it */
    /* For now, only default rules supported with ascii 
       regular expressions. */
    debug_printf ("Converting query string to wide\n");
    mstring_from_string (query_string, MAX_TRACK_LEN, 
			 ti->raw_metadata, CODESET_METADATA);
    for (rulep = m_global_rule_list; rulep->cmd; rulep++) {
	regmatch_t pmatch[MAX_SUBMATCHES+1];

	eflags = 0;
	if (rulep->cmd == PARSERULE_CMD_MATCH) {
	    debug_mprintf (m_("Testing match rule: ") m_S m_(" vs. ") m_S m_("\n"),
			    query_string, rulep->match);
	    if (rulep->flags & PARSERULE_SKIP) {
		rc = mregexec (rulep->reg, query_string, 0, NULL, eflags);
		if (rc != 0) {
		    /* Didn't match rule. */
		    continue;
		}
		/* GCS FIX: We need to return to the 
		    caller that the metadata should be dropped. */
		ti->save_track = FALSE;
		ti->have_track_info = 0;
		return;
	    } else if (rulep->flags & PARSERULE_SAVE) {
		rc = mregexec (rulep->reg, query_string, 0, NULL, eflags);
		if (rc != 0) {
		    /* Didn't match rule. */
		    if (!save_track_matched)
		        ti->save_track = FALSE;
		    continue;
		}
		if (!exclude_track_matched) {
		    ti->save_track = TRUE;
		    save_track_matched = TRUE;
		}
	    } else if (rulep->flags & PARSERULE_EXCLUDE) {
		rc = mregexec (rulep->reg, query_string, 0, NULL, eflags);
		if (rc == 0 && !save_track_matched) {
		    /* Rule matched => Exclude track */
		    ti->save_track = FALSE;
		    exclude_track_matched = TRUE;
		}
	    } else {
    		eflags = 0;
		rc = mregexec (rulep->reg, query_string, MAX_SUBMATCHES+1, 
			       pmatch, eflags);
		if (rc != 0) {
		    /* Didn't match rule. */
		    continue;
		}
		copy_rule_result (ti->artist, query_string, pmatch, rulep->artist_idx);
		copy_rule_result (ti->title, query_string, pmatch, rulep->title_idx);
		copy_rule_result (ti->album, query_string, pmatch, rulep->album_idx);
		ti->have_track_info = 1;
		compose_metadata (ti);
		/* GCS FIX: We don't have a track number. */
		debug_mprintf (m_("Parsed track info.\n")
			       m_("ARTIST: ") m_S m_("\n")
			       m_("TITLE: ")  m_S m_("\n")
			       m_("ALBUM: ")  m_S m_("\n"),
			       ti->artist, ti->title, ti->album);
		return;
	    }
	}
	else if (rulep->cmd == PARSERULE_CMD_SUBST) {
	    mchar subst_string[MAX_TRACK_LEN];
	    int used, left;
	    debug_mprintf (m_("Testing subst rule: ") m_S m_(" vs. ") m_S m_("\n"),
			    query_string, rulep->match);
	    rc = mregexec (rulep->reg, query_string, 1, pmatch, eflags);
	    if (rc != 0) {
		/* Didn't match rule. */
		continue;
	    }
	    /* Update the query string and continue. */
	    debug_printf ("Matched at (%d,%d)\n", 
			    pmatch[0].rm_so, pmatch[0].rm_eo);
	    mstrncpy (subst_string, query_string, pmatch[0].rm_so + 1);
	    debug_mprintf (m_("(1) subst_string = ") m_S m_("\n"), subst_string);
	    used = pmatch[0].rm_so;
	    left = MAX_TRACK_LEN - used;
	    mstrncpy (subst_string + used, rulep->subst, left);
	    debug_mprintf (m_("(2) subst_string = ") m_S m_("\n"), subst_string);
	    used += mstrlen (rulep->subst);
	    left = MAX_TRACK_LEN - used;
	    mstrncpy (subst_string + used, 
		      query_string + pmatch[0].rm_eo, left);
	    debug_mprintf (m_("(3) subst_string = ") m_S m_("\n"), subst_string);
	    mstrncpy (query_string, subst_string, MAX_TRACK_LEN);
	    debug_mprintf (m_("(4) query_string = ") m_S m_("\n"), query_string);
	}
    }
    debug_printf ("Fell through while parsing data...\n");
    mstrncpy (ti->title, query_string, MAX_TRACK_LEN);
    ti->have_track_info = 1;
    compose_metadata (ti);
}
