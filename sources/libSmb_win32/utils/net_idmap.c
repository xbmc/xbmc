/* 
   Samba Unix/Linux SMB client library 
   Distributed SMB/CIFS Server Management Utility 
   Copyright (C) 2003 Andrew Bartlett (abartlet@samba.org)

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "includes.h"
#include "utils/net.h"


/***********************************************************
 Helper function for net_idmap_dump. Dump one entry.
 **********************************************************/
static int net_idmap_dump_one_entry(TDB_CONTEXT *tdb,
				    TDB_DATA key,
				    TDB_DATA data,
				    void *unused)
{
	if (strcmp(key.dptr, "USER HWM") == 0) {
		printf("USER HWM %d\n", IVAL(data.dptr,0));
		return 0;
	}

	if (strcmp(key.dptr, "GROUP HWM") == 0) {
		printf("GROUP HWM %d\n", IVAL(data.dptr,0));
		return 0;
	}

	if (strncmp(key.dptr, "S-", 2) != 0)
		return 0;

	printf("%s %s\n", data.dptr, key.dptr);
	return 0;
}

/***********************************************************
 Dump the current idmap
 **********************************************************/
static int net_idmap_dump(int argc, const char **argv)
{
	TDB_CONTEXT *idmap_tdb;

	if ( argc != 1 )
		return net_help_idmap( argc, argv );

	idmap_tdb = tdb_open_log(argv[0], 0, TDB_DEFAULT, O_RDONLY, 0);

	if (idmap_tdb == NULL) {
		d_fprintf(stderr, "Could not open idmap: %s\n", argv[0]);
		return -1;
	}

	tdb_traverse(idmap_tdb, net_idmap_dump_one_entry, NULL);

	tdb_close(idmap_tdb);

	return 0;
}

/***********************************************************
 Fix up the HWMs after a idmap restore.
 **********************************************************/

struct hwms {
	BOOL ok;
	uid_t user_hwm;
	gid_t group_hwm;
};

static int net_idmap_find_max_id(TDB_CONTEXT *tdb, TDB_DATA key, TDB_DATA data,
				 void *handle)
{
	struct hwms *hwms = (struct hwms *)handle;
	void *idptr = NULL;
	BOOL isgid = False;
	int id;

	if (strncmp(key.dptr, "S-", 2) != 0)
		return 0;

	if (sscanf(data.dptr, "GID %d", &id) == 1) {
		idptr = (void *)&hwms->group_hwm;
		isgid = True;
	}

	if (sscanf(data.dptr, "UID %d", &id) == 1) {
		idptr = (void *)&hwms->user_hwm;
		isgid = False;
	}

	if (idptr == NULL) {
		d_fprintf(stderr, "Illegal idmap entry: [%s]->[%s]\n",
			 key.dptr, data.dptr);
		hwms->ok = False;
		return -1;
	}

	if (isgid) {
		if (hwms->group_hwm <= (gid_t)id) {
			hwms->group_hwm = (gid_t)(id+1);
		}
	} else {
		if (hwms->user_hwm <= (uid_t)id) {
			hwms->user_hwm = (uid_t)(id+1);
		}
	}

	return 0;
}

static NTSTATUS net_idmap_fixup_hwm(void)
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	TDB_CONTEXT *idmap_tdb;
	char *tdbfile = NULL;

	struct hwms hwms;
	struct hwms highest;

	if (!lp_idmap_uid(&hwms.user_hwm, &highest.user_hwm) ||
	    !lp_idmap_gid(&hwms.group_hwm, &highest.group_hwm)) {
		d_fprintf(stderr, "idmap range missing\n");
		return NT_STATUS_UNSUCCESSFUL;
	}

	tdbfile = SMB_STRDUP(lock_path("winbindd_idmap.tdb"));
	if (!tdbfile) {
		DEBUG(0, ("idmap_init: out of memory!\n"));
		return NT_STATUS_NO_MEMORY;
	}

	idmap_tdb = tdb_open_log(tdbfile, 0, TDB_DEFAULT, O_RDWR, 0);

	if (idmap_tdb == NULL) {
		d_fprintf(stderr, "Could not open idmap: %s\n", tdbfile);
		return NT_STATUS_NO_SUCH_FILE;
	}

	hwms.ok = True;

	tdb_traverse(idmap_tdb, net_idmap_find_max_id, &hwms);

	if (!hwms.ok) {
		goto done;
	}

	d_printf("USER HWM: %d  GROUP HWM: %d\n",
		 hwms.user_hwm, hwms.group_hwm);

	if (hwms.user_hwm >= highest.user_hwm) {
		d_fprintf(stderr, "Highest UID out of uid range\n");
		goto done;
	}

	if (hwms.group_hwm >= highest.group_hwm) {
		d_fprintf(stderr, "Highest GID out of gid range\n");
		goto done;
	}

	if ((tdb_store_int32(idmap_tdb, "USER HWM", (int32)hwms.user_hwm) != 0) ||
	    (tdb_store_int32(idmap_tdb, "GROUP HWM", (int32)hwms.group_hwm) != 0)) {
		d_fprintf(stderr, "Could not store HWMs\n");
		goto done;
	}

	result = NT_STATUS_OK;
 done:
	tdb_close(idmap_tdb);
	return result;
}

/***********************************************************
 Write entries from stdin to current local idmap
 **********************************************************/
static int net_idmap_restore(int argc, const char **argv)
{
	if (!idmap_init(lp_idmap_backend())) {
		d_fprintf(stderr, "Could not init idmap\n");
		return -1;
	}

	while (!feof(stdin)) {
		fstring line, sid_string, fmt_string;
		int len;
		unid_t id;
		int type = ID_EMPTY;
		DOM_SID sid;

		if (fgets(line, sizeof(line)-1, stdin) == NULL)
			break;

		len = strlen(line);

		if ( (len > 0) && (line[len-1] == '\n') )
			line[len-1] = '\0';

		/* Yuck - this is broken for sizeof(gid_t) != sizeof(int) */
		snprintf(fmt_string, sizeof(fmt_string), "GID %%d %%%us", FSTRING_LEN);
		if (sscanf(line, fmt_string, &id.gid, sid_string) == 2) {
			type = ID_GROUPID;
		}

		/* Yuck - this is broken for sizeof(uid_t) != sizeof(int) */

		snprintf(fmt_string, sizeof(fmt_string), "UID %%d %%%us", FSTRING_LEN);
		if (sscanf(line, fmt_string, &id.uid, sid_string) == 2) {
			type = ID_USERID;
		}

		if (type == ID_EMPTY) {
			d_printf("ignoring invalid line [%s]\n", line);
			continue;
		}

		if (!string_to_sid(&sid, sid_string)) {
			d_printf("ignoring invalid sid [%s]\n", sid_string);
			continue;
		}

		if (!NT_STATUS_IS_OK(idmap_set_mapping(&sid, id, type))) {
			d_fprintf(stderr, "Could not set mapping of %s %lu to sid %s\n",
				 (type == ID_GROUPID) ? "GID" : "UID",
				 (type == ID_GROUPID) ? (unsigned long)id.gid:
				 (unsigned long)id.uid, 
				 sid_string_static(&sid));
			continue;
		}
				 
	}

	idmap_close();

	return NT_STATUS_IS_OK(net_idmap_fixup_hwm()) ? 0 : -1;
}

/***********************************************************
 Delete a SID mapping from a winbindd_idmap.tdb
 **********************************************************/
static int net_idmap_delete(int argc, const char **argv)
{
	TDB_CONTEXT *idmap_tdb;
	TDB_DATA key, data;
	fstring sid;

	if (argc != 2)
		return net_help_idmap(argc, argv);

	idmap_tdb = tdb_open_log(argv[0], 0, TDB_DEFAULT, O_RDWR, 0);

	if (idmap_tdb == NULL) {
		d_fprintf(stderr, "Could not open idmap: %s\n", argv[0]);
		return -1;
	}

	fstrcpy(sid, argv[1]);

	if (strncmp(sid, "S-1-5-", strlen("S-1-5-")) != 0) {
		d_fprintf(stderr, "Can only delete SIDs, %s is does not start with "
			 "S-1-5-\n", sid);
		return -1;
	}

	key.dptr = sid;
	key.dsize = strlen(key.dptr)+1;

	data = tdb_fetch(idmap_tdb, key);

	if (data.dptr == NULL) {
		d_fprintf(stderr, "Could not find sid %s\n", argv[1]);
		return -1;
	}

	if (tdb_delete(idmap_tdb, key) != 0) {
		d_fprintf(stderr, "Could not delete key %s\n", argv[1]);
		return -1;
	}

	if (tdb_delete(idmap_tdb, data) != 0) {
		d_fprintf(stderr, "Could not delete key %s\n", data.dptr);
		return -1;
	}

	return 0;
}


int net_help_idmap(int argc, const char **argv)
{
	d_printf("net idmap dump <tdbfile>"\
		 "\n  Dump current id mapping\n");

	d_printf("net idmap restore"\
		 "\n  Restore entries from stdin to current local idmap\n");

	/* Deliberately *not* document net idmap delete */

	return -1;
}

/***********************************************************
 Look at the current idmap
 **********************************************************/
int net_idmap(int argc, const char **argv)
{
	struct functable func[] = {
		{"dump", net_idmap_dump},
		{"restore", net_idmap_restore},
		{"delete", net_idmap_delete},
		{"help", net_help_idmap},
		{NULL, NULL}
	};

	return net_run_function(argc, argv, func, net_help_idmap);
}


