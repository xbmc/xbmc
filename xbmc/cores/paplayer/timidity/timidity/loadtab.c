/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2004 Masanao Izumo <iz@onicos.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    loadtab.c - written by Nando Santagata <lac0658@iperbole.bologna.it>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "timidity.h"
#include "common.h"
#include "tables.h"
#include "controls.h"

int load_table(char *file)
{
	FILE *fp;
	char tmp[1024], *value;
	int i = 0;
	
	if ((fp = fopen(file, "r")) == NULL) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"Can't read %s %s\n", file, strerror(errno));
		return -1;
	}
	while (fgets(tmp, sizeof(tmp), fp)) {
		if (strchr(tmp, '#'))
			continue;
		if (! (value = strtok(tmp, ", \n")))
			continue;
		do {
			freq_table_zapped[i++] = atoi(value);
			if (i == 128) {
				fclose(fp);
				return 0;
			}
		} while (value = strtok(NULL, ", \n"));
	}
	fclose(fp);
	return 0;
}

