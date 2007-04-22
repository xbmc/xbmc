#include "links.h"

#ifdef FORM_SAVE

void
db_error(char *str)
{
	internal("Your computer is on fire!!!");
}

GDBM_FILE
db_open(unsigned char *filename, unsigned char *m)
{
	GDBM_FILE db = NULL;
	int mode = GDBM_READER;

	if (m) {
		if (m[0] == 'r' && m[1] == 'w')  mode = (GDBM_WRCREAT | GDBM_SYNC);
		else if (m[0] == 'w' && m[1] == '+') mode = (GDBM_NEWDB | GDBM_SYNC);
	}
	db = gdbm_open(filename, 0, mode, S_IRUSR | S_IWUSR, db_error);
	return db;
}

void
db_close(GDBM_FILE db, int re_org)
{
	static char reorg_magic[]="\001x93ahg7we83\001";
	static unsigned char f[64];
	unsigned char *s;
	int l, i = 0;

	if (! db) return;
	if (re_org > 0) {
		if (! db_fetch(db, reorg_magic, &s, &l)) {
			i = (int) strtol(s, NULL, 10);
			mem_free(s);
		}
		if (i >= re_org) { gdbm_reorganize(db); i = 0; }
		sprintf(f, "%d", ++i);
		db_insert(db, reorg_magic, f, strlen(f) + 1);
	}
	gdbm_close(db);
}

int
db_insert(GDBM_FILE db, unsigned char *key, unsigned char *data, int dalen)
{
	datum db_key = {key, 0};
	datum db_data = {data, dalen};

	if (! db || ! key || ! key[0] || ! data || dalen <= 0) return -1;
	db_key.dsize = strlen(key) + 1;
	return gdbm_store(db, db_key, db_data, GDBM_REPLACE);
}

int
db_fetch(GDBM_FILE db, unsigned char *key, unsigned char **data, int *dalen)
{
	datum db_key = { key, 0 };
	datum db_data;
	unsigned char *d;
	int l;

	if (data) *data = NULL;
	if (dalen) *dalen = 0;

	if (! db || ! key || ! key[0] || ! data || ! dalen) return -1;
	db_key.dsize = strlen(key) + 1; 
	db_data = gdbm_fetch(db, db_key);
	if (db_data.dptr) {
		l = db_data.dsize;
		if (! (d = mem_alloc(l + 1))) { free(db_data.dptr); return -1; }
		memcpy(d, db_data.dptr, l);
		d[l] = '\0';	/* just in case ... */
		*data = d;
		*dalen = l;
		free(db_data.dptr);
		return 0;
	}
	return -1;
}			

void
db_fetchall(GDBM_FILE db, void (*f)(void *, unsigned char *, unsigned char *, int), void *p)
{
	datum db_key, db_data, ok;
  
	if (!db || !f) return;
	db_key = gdbm_firstkey(db);
	while (db_key.dptr != NULL) {
		db_data = gdbm_fetch(db, db_key);
		if (! db_data.dptr) {
			free(db_key.dptr);                                    
			break;
		}
		f(p, db_key.dptr, db_data.dptr, db_data.dsize);	/* unlike db_fetch, value may not be null terminated */
		free(db_data.dptr);
		ok = db_key;
		db_key = gdbm_nextkey(db, db_key);
		free(ok.dptr);
	}
}

int
db_exists(GDBM_FILE db, unsigned char *key)
{
	datum db_key = { key, 0 };

	if (! db || ! key || ! key[0]) return 0;
	db_key.dsize = strlen(key) + 1;
	return gdbm_exists(db, db_key);
}

int
db_delete(GDBM_FILE db, unsigned char *key)
{
	datum db_key = { key, 0 };

	if (! db || ! key || ! key[0]) return -1;
	db_key.dsize = strlen(key) + 1;
	return gdbm_delete(db, db_key);
}

#endif
