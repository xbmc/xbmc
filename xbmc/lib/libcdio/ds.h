/*
    $Id$

    Copyright (C) 2000, 2004 Herbert Valerio Riedel <hvr@gnu.org>

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
/* Note: this header will is slated to get removed and libcdio will use 
   glib.h routines instead. 
*/

#ifndef __CDIO_DS_H__
#define __CDIO_DS_H__

#include <types.h>

/* opaque... */
typedef struct _CdioList CdioList;
typedef struct _CdioListNode CdioListNode;

typedef int (*_cdio_list_cmp_func) (void *data1, void *data2);

typedef int (*_cdio_list_iterfunc) (void *data, void *user_data);

/* methods */
CdioList *_cdio_list_new (void);

void _cdio_list_free (CdioList *list, int free_data);

unsigned _cdio_list_length (const CdioList *list);

void _cdio_list_prepend (CdioList *list, void *data);

void _cdio_list_append (CdioList *list, void *data);

void _cdio_list_foreach (CdioList *list, _cdio_list_iterfunc func, void *user_data);

CdioListNode *_cdio_list_find (CdioList *list, _cdio_list_iterfunc cmp_func, void *user_data);

#define _CDIO_LIST_FOREACH(node, list) \
 for (node = _cdio_list_begin (list); node; node = _cdio_list_node_next (node))

/* node ops */

CdioListNode *_cdio_list_begin (const CdioList *list);

CdioListNode *_cdio_list_end (CdioList *list);

CdioListNode *_cdio_list_node_next (CdioListNode *node);

void _cdio_list_node_free (CdioListNode *node, int free_data);

void *_cdio_list_node_data (CdioListNode *node);

#endif /* __CDIO_DS_H__ */

/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */

