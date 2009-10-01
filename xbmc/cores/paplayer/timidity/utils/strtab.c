/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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
#include "mblock.h"
#include "strtab.h"

void init_string_table(StringTable *stab)
{
    memset(stab, 0, sizeof(StringTable));
}

StringTableNode *put_string_table(StringTable *stab, char *str, int len)
{
    StringTableNode *p;

    p = new_segment(&stab->pool, sizeof(StringTableNode) + len + 1);
    if(p == NULL)
	return NULL;
    p->next = NULL;
    if(str != NULL)
    {
	memcpy(p->string, str, len);
	p->string[len] = '\0';
    }
	
    if(stab->head == NULL)
    {
	stab->head = stab->tail = p;
	stab->nstring = 1;
    }
    else
    {
	stab->nstring++;
	stab->tail = stab->tail->next = p;
    }
    return p;
}

char **make_string_array(StringTable *stab)
{
    char **table, *u;
    int i, n, s;
    StringTableNode *p;

    n = stab->nstring;
    if(n == 0)
	return NULL;
    if((table = (char **)safe_malloc((n + 1) * sizeof(char *))) == NULL)
	return NULL;

    s = 0;
    for(p = stab->head; p; p = p->next)
	s += strlen(p->string) + 1;

    if((u = (char *)safe_malloc(s)) == NULL)
    {
	free(table);
	return NULL;
    }

    for(i = 0, p = stab->head; p; i++, p = p->next)
    {
	int len;

	len = strlen(p->string) + 1;
	table[i] = u;
	memcpy(u, p->string, len);
	u += len;
    }
    table[i] = NULL;
    delete_string_table(stab);
    return table;
}

void delete_string_table(StringTable *stab)
{
    reuse_mblock(&stab->pool);
    init_string_table(stab);
}
