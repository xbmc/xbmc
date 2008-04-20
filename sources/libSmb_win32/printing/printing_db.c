/* 
   Unix SMB/Netbios implementation.
   Version 3.0
   printing backend routines
   Copyright (C) Andrew Tridgell 1992-2000
   Copyright (C) Jeremy Allison 2002
   
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

#include "includes.h"
#include "printing.h"

static struct tdb_print_db *print_db_head;

/****************************************************************************
  Function to find or create the printer specific job tdb given a printername.
  Limits the number of tdb's open to MAX_PRINT_DBS_OPEN.
****************************************************************************/

struct tdb_print_db *get_print_db_byname(const char *printername)
{
	struct tdb_print_db *p = NULL, *last_entry = NULL;
	int num_open = 0;
	pstring printdb_path;
	BOOL done_become_root = False;

	SMB_ASSERT(printername != NULL);

	for (p = print_db_head, last_entry = print_db_head; p; p = p->next) {
		/* Ensure the list terminates... JRA. */
		SMB_ASSERT(p->next != print_db_head);

		if (p->tdb && strequal(p->printer_name, printername)) {
			DLIST_PROMOTE(print_db_head, p);
			p->ref_count++;
			return p;
		}
		num_open++;
		last_entry = p;
	}

	/* Not found. */
	if (num_open >= MAX_PRINT_DBS_OPEN) {
		/* Try and recycle the last entry. */
		if (print_db_head && last_entry) {
			DLIST_PROMOTE(print_db_head, last_entry);
		}

		for (p = print_db_head; p; p = p->next) {
			if (p->ref_count)
				continue;
			if (p->tdb) {
				if (tdb_close(print_db_head->tdb)) {
					DEBUG(0,("get_print_db: Failed to close tdb for printer %s\n",
								print_db_head->printer_name ));
					return NULL;
				}
			}
			p->tdb = NULL;
			p->ref_count = 0;
			memset(p->printer_name, '\0', sizeof(p->printer_name));
			break;
		}
		if (p && print_db_head) {
			DLIST_PROMOTE(print_db_head, p);
			p = print_db_head;
		}
	}
       
	if (!p)	{
		/* Create one. */
		p = SMB_MALLOC_P(struct tdb_print_db);
		if (!p) {
			DEBUG(0,("get_print_db: malloc fail !\n"));
			return NULL;
		}
		ZERO_STRUCTP(p);
		DLIST_ADD(print_db_head, p);
	}

	pstrcpy(printdb_path, lock_path("printing/"));
	pstrcat(printdb_path, printername);
	pstrcat(printdb_path, ".tdb");

	if (geteuid() != 0) {
		become_root();
		done_become_root = True;
	}

	p->tdb = tdb_open_log(printdb_path, 5000, TDB_DEFAULT, O_RDWR|O_CREAT, 
		0600);

	if (done_become_root)
		unbecome_root();

	if (!p->tdb) {
		DEBUG(0,("get_print_db: Failed to open printer backend database %s.\n",
					printdb_path ));
		DLIST_REMOVE(print_db_head, p);
		SAFE_FREE(p);
		return NULL;
	}
	fstrcpy(p->printer_name, printername);
	p->ref_count++;
	return p;
}

/***************************************************************************
 Remove a reference count.
****************************************************************************/

void release_print_db( struct tdb_print_db *pdb)
{
	pdb->ref_count--;
	SMB_ASSERT(pdb->ref_count >= 0);
}

/***************************************************************************
 Close all open print db entries.
****************************************************************************/

void close_all_print_db(void)
{
	struct tdb_print_db *p = NULL, *next_p = NULL;

	for (p = print_db_head; p; p = next_p) {
		next_p = p->next;

		if (p->tdb)
			tdb_close(p->tdb);
		DLIST_REMOVE(print_db_head, p);
		ZERO_STRUCTP(p);
		SAFE_FREE(p);
	}
}


/****************************************************************************
 Fetch and clean the pid_t record list for all pids interested in notify
 messages. data needs freeing on exit.
****************************************************************************/

TDB_DATA get_printer_notify_pid_list(TDB_CONTEXT *tdb, const char *printer_name, BOOL cleanlist)
{
	TDB_DATA data;
	size_t i;

	ZERO_STRUCT(data);

	data = tdb_fetch_bystring( tdb, NOTIFY_PID_LIST_KEY );

	if (!data.dptr) {
		ZERO_STRUCT(data);
		return data;
	}

	if (data.dsize % 8) {
		DEBUG(0,("get_printer_notify_pid_list: Size of record for printer %s not a multiple of 8 !\n", printer_name ));
		tdb_delete_bystring(tdb, NOTIFY_PID_LIST_KEY );
		SAFE_FREE(data.dptr);
		ZERO_STRUCT(data);
		return data;
	}

	if (!cleanlist)
		return data;

	/*
	 * Weed out all dead entries.
	 */

	for( i = 0; i < data.dsize; i += 8) {
		pid_t pid = (pid_t)IVAL(data.dptr, i);

		if (pid == sys_getpid())
			continue;

		/* Entry is dead if process doesn't exist or refcount is zero. */

		while ((i < data.dsize) && ((IVAL(data.dptr, i + 4) == 0) || !process_exists_by_pid(pid))) {

			/* Refcount == zero is a logic error and should never happen. */
			if (IVAL(data.dptr, i + 4) == 0) {
				DEBUG(0,("get_printer_notify_pid_list: Refcount == 0 for pid = %u printer %s !\n",
							(unsigned int)pid, printer_name ));
			}

			if (data.dsize - i > 8)
				memmove( &data.dptr[i], &data.dptr[i+8], data.dsize - i - 8);
			data.dsize -= 8;
		}
	}

	return data;
}


