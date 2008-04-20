/* 
   Unix SMB/CIFS implementation.
   printcap parsing
   Copyright (C) Karl Auer 1993-1998

   Re-working by Martin Kiff, 1994
   
   Re-written again by Andrew Tridgell

   Modified for SVID support by Norm Jacobs, 1997

   Modified for CUPS support by Michael Sweet, 1999
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
 *  This module contains code to parse and cache printcap data, possibly
 *  in concert with the CUPS/SYSV/AIX-specific code found elsewhere.
 *
 *  The way this module looks at the printcap file is very simplistic.
 *  Only the local printcap file is inspected (no searching of NIS
 *  databases etc).
 *
 *  There are assumed to be one or more printer names per record, held
 *  as a set of sub-fields separated by vertical bar symbols ('|') in the
 *  first field of the record. The field separator is assumed to be a colon
 *  ':' and the record separator a newline.
 * 
 *  Lines ending with a backspace '\' are assumed to flag that the following
 *  line is a continuation line so that a set of lines can be read as one
 *  printcap entry.
 *
 *  A line stating with a hash '#' is assumed to be a comment and is ignored
 *  Comments are discarded before the record is strung together from the
 *  set of continuation lines.
 *
 *  Opening a pipe for "lpc status" and reading that would probably 
 *  be pretty effective. Code to do this already exists in the freely
 *  distributable PCNFS server code.
 *
 *  Modified to call SVID/XPG4 support if printcap name is set to "lpstat"
 *  in smb.conf under Solaris.
 *
 *  Modified to call CUPS support if printcap name is set to "cups"
 *  in smb.conf.
 *
 *  Modified to call iPrint support if printcap name is set to "iprint"
 *  in smb.conf.
 */

#include "includes.h"


typedef struct pcap_cache {
	char *name;
	char *comment;
	struct pcap_cache *next;
} pcap_cache_t;

static pcap_cache_t *pcap_cache = NULL;

BOOL pcap_cache_add(const char *name, const char *comment)
{
	pcap_cache_t *p;

	if (name == NULL || ((p = SMB_MALLOC_P(pcap_cache_t)) == NULL))
		return False;

	p->name = SMB_STRDUP(name);
	p->comment = (comment && *comment) ? SMB_STRDUP(comment) : NULL;

	p->next = pcap_cache;
	pcap_cache = p;

	return True;
}

static void pcap_cache_destroy(pcap_cache_t *cache)
{
	pcap_cache_t *p, *next;

	for (p = cache; p != NULL; p = next) {
		next = p->next;

		SAFE_FREE(p->name);
		SAFE_FREE(p->comment);
		SAFE_FREE(p);
	}
}

BOOL pcap_cache_loaded(void)
{
	return (pcap_cache != NULL);
}

void pcap_cache_reload(void)
{
	const char *pcap_name = lp_printcapname();
	BOOL pcap_reloaded = False;
	pcap_cache_t *tmp_cache = NULL;
	XFILE *pcap_file;
	char *pcap_line;

	DEBUG(3, ("reloading printcap cache\n"));

	/* only go looking if no printcap name supplied */
	if (pcap_name == NULL || *pcap_name == 0) {
		DEBUG(0, ("No printcap file name configured!\n"));
		return;
	}

	tmp_cache = pcap_cache;
	pcap_cache = NULL;

#ifdef HAVE_CUPS
	if (strequal(pcap_name, "cups")) {
		pcap_reloaded = cups_cache_reload();
		goto done;
	}
#endif

#ifdef HAVE_IPRINT
	if (strequal(pcap_name, "iprint")) {
		pcap_reloaded = iprint_cache_reload();
		goto done;
	}
#endif

#if defined(SYSV) || defined(HPUX)
	if (strequal(pcap_name, "lpstat")) {
		pcap_reloaded = sysv_cache_reload();
		goto done;
	}
#endif

#ifdef AIX
	if (strstr_m(pcap_name, "/qconfig") != NULL) {
		pcap_reloaded = aix_cache_reload();
		goto done;
	}
#endif

	/* handle standard printcap - moved from pcap_printer_fn() */

	if ((pcap_file = x_fopen(pcap_name, O_RDONLY, 0)) == NULL) {
		DEBUG(0, ("Unable to open printcap file %s for read!\n", pcap_name));
		goto done;
	}

	for (; (pcap_line = fgets_slash(NULL, sizeof(pstring), pcap_file)) != NULL; safe_free(pcap_line)) {
		pstring name, comment;
		char *p, *q;

		if (*pcap_line == '#' || *pcap_line == 0)
			continue;

		/* now we have a real printer line - cut at the first : */      
		if ((p = strchr_m(pcap_line, ':')) != NULL)
			*p = 0;
      
		/*
		 * now find the most likely printer name and comment 
		 * this is pure guesswork, but it's better than nothing
		 */
		for (*name = *comment = 0, p = pcap_line; p != NULL; p = q) {
			BOOL has_punctuation;

			if ((q = strchr_m(p, '|')) != NULL)
				*q++ = 0;

			has_punctuation = (strchr_m(p, ' ') ||
			                   strchr_m(p, '\t') ||
			                   strchr_m(p, '(') ||
			                   strchr_m(p, ')'));

			if (strlen(p) > strlen(comment) && has_punctuation) {
				pstrcpy(comment, p);
				continue;
			}

			if (strlen(p) <= MAXPRINTERLEN &&
			    strlen(p) > strlen(name) && !has_punctuation) {
				if (!*comment)
					pstrcpy(comment, name);

				pstrcpy(name, p);
				continue;
			}

			if (!strchr_m(comment, ' ') &&
			    strlen(p) > strlen(comment)) {
				pstrcpy(comment, p);
				continue;
			}
		}

		comment[60] = 0;
		name[MAXPRINTERLEN] = 0;

		if (*name && !pcap_cache_add(name, comment)) {
			x_fclose(pcap_file);
			goto done;
		}
	}

	x_fclose(pcap_file);
	pcap_reloaded = True;

done:
	DEBUG(3, ("reload status: %s\n", (pcap_reloaded) ? "ok" : "error"));

	if (pcap_reloaded)
		pcap_cache_destroy(tmp_cache);
	else {
		pcap_cache_destroy(pcap_cache);
		pcap_cache = tmp_cache;
	}

	return;
}


BOOL pcap_printername_ok(const char *printername)
{
	pcap_cache_t *p;

	for (p = pcap_cache; p != NULL; p = p->next)
		if (strequal(p->name, printername))
			return True;

	return False;
}

/***************************************************************************
run a function on each printer name in the printcap file. The function is 
passed the primary name and the comment (if possible). Note the fn() takes
strings in DOS codepage. This means the xxx_printer_fn() calls must be fixed
to return DOS codepage. FIXME !! JRA.

XXX: I'm not sure if this comment still applies.. Anyone?  -Rob
***************************************************************************/
void pcap_printer_fn(void (*fn)(char *, char *))
{
	pcap_cache_t *p;

	for (p = pcap_cache; p != NULL; p = p->next)
		fn(p->name, p->comment);

	return;
}
