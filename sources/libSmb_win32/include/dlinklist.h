/* 
   Unix SMB/CIFS implementation.
   some simple double linked list macros
   Copyright (C) Andrew Tridgell 1998
   
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

/* To use these macros you must have a structure containing a next and
   prev pointer */


/* hook into the front of the list */
#define DLIST_ADD(list, p) \
{ \
        if (!(list)) { \
		(list) = (p); \
		(p)->next = (p)->prev = NULL; \
	} else { \
		(list)->prev = (p); \
		(p)->next = (list); \
		(p)->prev = NULL; \
		(list) = (p); \
	}\
}

/* remove an element from a list - element doesn't have to be in list. */
#define DLIST_REMOVE(list, p) \
{ \
	if ((p) == (list)) { \
		(list) = (p)->next; \
		if (list) (list)->prev = NULL; \
	} else { \
		if ((p)->prev) (p)->prev->next = (p)->next; \
		if ((p)->next) (p)->next->prev = (p)->prev; \
	} \
	if ((p) != (list)) (p)->next = (p)->prev = NULL; \
}

/* promote an element to the top of the list */
#define DLIST_PROMOTE(list, p) \
{ \
          DLIST_REMOVE(list, p) \
          DLIST_ADD(list, p) \
}

/* hook into the end of the list - needs a tmp pointer */
#define DLIST_ADD_END(list, p, tmp) \
{ \
		if (!(list)) { \
			(list) = (p); \
			(p)->next = (p)->prev = NULL; \
		} else { \
			for ((tmp) = (list); (tmp)->next; (tmp) = (tmp)->next) ; \
			(tmp)->next = (p); \
			(p)->next = NULL; \
			(p)->prev = (tmp); \
		} \
}

/* insert 'p' after the given element 'el' in a list. If el is NULL then
   this is the same as a DLIST_ADD() */
#define DLIST_ADD_AFTER(list, p, el) \
do { \
        if (!(list) || !(el)) { \
		DLIST_ADD(list, p); \
	} else { \
		p->prev = el; \
		p->next = el->next; \
		el->next = p; \
		if (p->next) p->next->prev = p; \
	}\
} while (0)

/* demote an element to the top of the list, needs a tmp pointer */
#define DLIST_DEMOTE(list, p, tmp) \
{ \
		DLIST_REMOVE(list, p) \
		DLIST_ADD_END(list, p, tmp) \
}
