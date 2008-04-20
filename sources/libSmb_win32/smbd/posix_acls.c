/*
   Unix SMB/CIFS implementation.
   SMB NT Security Descriptor / Unix permission conversion.
   Copyright (C) Jeremy Allison 1994-2000.
   Copyright (C) Andreas Gruenbacher 2002.

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

extern struct current_user current_user;
extern struct generic_mapping file_generic_mapping;

#undef  DBGC_CLASS
#define DBGC_CLASS DBGC_ACLS

/****************************************************************************
 Data structures representing the internal ACE format.
****************************************************************************/

enum ace_owner {UID_ACE, GID_ACE, WORLD_ACE};
enum ace_attribute {ALLOW_ACE, DENY_ACE}; /* Used for incoming NT ACLS. */

typedef union posix_id {
		uid_t uid;
		gid_t gid;
		int world;
} posix_id;

typedef struct canon_ace {
	struct canon_ace *next, *prev;
	SMB_ACL_TAG_T type;
	mode_t perms; /* Only use S_I(R|W|X)USR mode bits here. */
	DOM_SID trustee;
	enum ace_owner owner_type;
	enum ace_attribute attr;
	posix_id unix_ug; 
	BOOL inherited;
} canon_ace;

#define ALL_ACE_PERMS (S_IRUSR|S_IWUSR|S_IXUSR)

/*
 * EA format of user.SAMBA_PAI (Samba_Posix_Acl_Interitance)
 * attribute on disk.
 *
 * |  1   |  1   |   2         |         2           |  .... 
 * +------+------+-------------+---------------------+-------------+--------------------+
 * | vers | flag | num_entries | num_default_entries | ..entries.. | default_entries... |
 * +------+------+-------------+---------------------+-------------+--------------------+
 */

#define PAI_VERSION_OFFSET	0
#define PAI_FLAG_OFFSET		1
#define PAI_NUM_ENTRIES_OFFSET	2
#define PAI_NUM_DEFAULT_ENTRIES_OFFSET	4
#define PAI_ENTRIES_BASE	6

#define PAI_VERSION		1
#define PAI_ACL_FLAG_PROTECTED	0x1
#define PAI_ENTRY_LENGTH	5

/*
 * In memory format of user.SAMBA_PAI attribute.
 */

struct pai_entry {
	struct pai_entry *next, *prev;
	enum ace_owner owner_type;
	posix_id unix_ug; 
};
	
struct pai_val {
	BOOL pai_protected;
	unsigned int num_entries;
	struct pai_entry *entry_list;
	unsigned int num_def_entries;
	struct pai_entry *def_entry_list;
};

/************************************************************************
 Return a uint32 of the pai_entry principal.
************************************************************************/

static uint32 get_pai_entry_val(struct pai_entry *paie)
{
	switch (paie->owner_type) {
		case UID_ACE:
			DEBUG(10,("get_pai_entry_val: uid = %u\n", (unsigned int)paie->unix_ug.uid ));
			return (uint32)paie->unix_ug.uid;
		case GID_ACE:
			DEBUG(10,("get_pai_entry_val: gid = %u\n", (unsigned int)paie->unix_ug.gid ));
			return (uint32)paie->unix_ug.gid;
		case WORLD_ACE:
		default:
			DEBUG(10,("get_pai_entry_val: world ace\n"));
			return (uint32)-1;
	}
}

/************************************************************************
 Return a uint32 of the entry principal.
************************************************************************/

static uint32 get_entry_val(canon_ace *ace_entry)
{
	switch (ace_entry->owner_type) {
		case UID_ACE:
			DEBUG(10,("get_entry_val: uid = %u\n", (unsigned int)ace_entry->unix_ug.uid ));
			return (uint32)ace_entry->unix_ug.uid;
		case GID_ACE:
			DEBUG(10,("get_entry_val: gid = %u\n", (unsigned int)ace_entry->unix_ug.gid ));
			return (uint32)ace_entry->unix_ug.gid;
		case WORLD_ACE:
		default:
			DEBUG(10,("get_entry_val: world ace\n"));
			return (uint32)-1;
	}
}

/************************************************************************
 Count the inherited entries.
************************************************************************/

static unsigned int num_inherited_entries(canon_ace *ace_list)
{
	unsigned int num_entries = 0;

	for (; ace_list; ace_list = ace_list->next)
		if (ace_list->inherited)
			num_entries++;
	return num_entries;
}

/************************************************************************
 Create the on-disk format. Caller must free.
************************************************************************/

static char *create_pai_buf(canon_ace *file_ace_list, canon_ace *dir_ace_list, BOOL pai_protected, size_t *store_size)
{
	char *pai_buf = NULL;
	canon_ace *ace_list = NULL;
	char *entry_offset = NULL;
	unsigned int num_entries = 0;
	unsigned int num_def_entries = 0;

	for (ace_list = file_ace_list; ace_list; ace_list = ace_list->next)
		if (ace_list->inherited)
			num_entries++;

	for (ace_list = dir_ace_list; ace_list; ace_list = ace_list->next)
		if (ace_list->inherited)
			num_def_entries++;

	DEBUG(10,("create_pai_buf: num_entries = %u, num_def_entries = %u\n", num_entries, num_def_entries ));

	*store_size = PAI_ENTRIES_BASE + ((num_entries + num_def_entries)*PAI_ENTRY_LENGTH);

	pai_buf = SMB_MALLOC(*store_size);
	if (!pai_buf) {
		return NULL;
	}

	/* Set up the header. */
	memset(pai_buf, '\0', PAI_ENTRIES_BASE);
	SCVAL(pai_buf,PAI_VERSION_OFFSET,PAI_VERSION);
	SCVAL(pai_buf,PAI_FLAG_OFFSET,(pai_protected ? PAI_ACL_FLAG_PROTECTED : 0));
	SSVAL(pai_buf,PAI_NUM_ENTRIES_OFFSET,num_entries);
	SSVAL(pai_buf,PAI_NUM_DEFAULT_ENTRIES_OFFSET,num_def_entries);

	entry_offset = pai_buf + PAI_ENTRIES_BASE;

	for (ace_list = file_ace_list; ace_list; ace_list = ace_list->next) {
		if (ace_list->inherited) {
			uint8 type_val = (unsigned char)ace_list->owner_type;
			uint32 entry_val = get_entry_val(ace_list);

			SCVAL(entry_offset,0,type_val);
			SIVAL(entry_offset,1,entry_val);
			entry_offset += PAI_ENTRY_LENGTH;
		}
	}

	for (ace_list = dir_ace_list; ace_list; ace_list = ace_list->next) {
		if (ace_list->inherited) {
			uint8 type_val = (unsigned char)ace_list->owner_type;
			uint32 entry_val = get_entry_val(ace_list);

			SCVAL(entry_offset,0,type_val);
			SIVAL(entry_offset,1,entry_val);
			entry_offset += PAI_ENTRY_LENGTH;
		}
	}

	return pai_buf;
}

/************************************************************************
 Store the user.SAMBA_PAI attribute on disk.
************************************************************************/

static void store_inheritance_attributes(files_struct *fsp, canon_ace *file_ace_list,
					canon_ace *dir_ace_list, BOOL pai_protected)
{
	int ret;
	size_t store_size;
	char *pai_buf;

	if (!lp_map_acl_inherit(SNUM(fsp->conn)))
		return;

	/*
	 * Don't store if this ACL isn't protected and
	 * none of the entries in it are marked as inherited.
	 */

	if (!pai_protected && num_inherited_entries(file_ace_list) == 0 && num_inherited_entries(dir_ace_list) == 0) {
		/* Instead just remove the attribute if it exists. */
		if (fsp->fh->fd != -1)
			SMB_VFS_FREMOVEXATTR(fsp, fsp->fh->fd, SAMBA_POSIX_INHERITANCE_EA_NAME);
		else
			SMB_VFS_REMOVEXATTR(fsp->conn, fsp->fsp_name, SAMBA_POSIX_INHERITANCE_EA_NAME);
		return;
	}

	pai_buf = create_pai_buf(file_ace_list, dir_ace_list, pai_protected, &store_size);

	if (fsp->fh->fd != -1)
		ret = SMB_VFS_FSETXATTR(fsp, fsp->fh->fd, SAMBA_POSIX_INHERITANCE_EA_NAME,
				pai_buf, store_size, 0);
	else
		ret = SMB_VFS_SETXATTR(fsp->conn,fsp->fsp_name, SAMBA_POSIX_INHERITANCE_EA_NAME,
				pai_buf, store_size, 0);

	SAFE_FREE(pai_buf);

	DEBUG(10,("store_inheritance_attribute:%s for file %s\n", pai_protected ? " (protected)" : "", fsp->fsp_name));
	if (ret == -1 && !no_acl_syscall_error(errno))
		DEBUG(1,("store_inheritance_attribute: Error %s\n", strerror(errno) ));
}

/************************************************************************
 Delete the in memory inheritance info.
************************************************************************/

static void free_inherited_info(struct pai_val *pal)
{
	if (pal) {
		struct pai_entry *paie, *paie_next;
		for (paie = pal->entry_list; paie; paie = paie_next) {
			paie_next = paie->next;
			SAFE_FREE(paie);
		}
		for (paie = pal->def_entry_list; paie; paie = paie_next) {
			paie_next = paie->next;
			SAFE_FREE(paie);
		}
		SAFE_FREE(pal);
	}
}

/************************************************************************
 Was this ACL protected ?
************************************************************************/

static BOOL get_protected_flag(struct pai_val *pal)
{
	if (!pal)
		return False;
	return pal->pai_protected;
}

/************************************************************************
 Was this ACE inherited ?
************************************************************************/

static BOOL get_inherited_flag(struct pai_val *pal, canon_ace *ace_entry, BOOL default_ace)
{
	struct pai_entry *paie;

	if (!pal)
		return False;

	/* If the entry exists it is inherited. */
	for (paie = (default_ace ? pal->def_entry_list : pal->entry_list); paie; paie = paie->next) {
		if (ace_entry->owner_type == paie->owner_type &&
				get_entry_val(ace_entry) == get_pai_entry_val(paie))
			return True;
	}
	return False;
}

/************************************************************************
 Ensure an attribute just read is valid.
************************************************************************/

static BOOL check_pai_ok(char *pai_buf, size_t pai_buf_data_size)
{
	uint16 num_entries;
	uint16 num_def_entries;

	if (pai_buf_data_size < PAI_ENTRIES_BASE) {
		/* Corrupted - too small. */
		return False;
	}

	if (CVAL(pai_buf,PAI_VERSION_OFFSET) != PAI_VERSION)
		return False;

	num_entries = SVAL(pai_buf,PAI_NUM_ENTRIES_OFFSET);
	num_def_entries = SVAL(pai_buf,PAI_NUM_DEFAULT_ENTRIES_OFFSET);

	/* Check the entry lists match. */
	/* Each entry is 5 bytes (type plus 4 bytes of uid or gid). */

	if (((num_entries + num_def_entries)*PAI_ENTRY_LENGTH) + PAI_ENTRIES_BASE != pai_buf_data_size)
		return False;

	return True;
}


/************************************************************************
 Convert to in-memory format.
************************************************************************/

static struct pai_val *create_pai_val(char *buf, size_t size)
{
	char *entry_offset;
	struct pai_val *paiv = NULL;
	int i;

	if (!check_pai_ok(buf, size))
		return NULL;

	paiv = SMB_MALLOC_P(struct pai_val);
	if (!paiv)
		return NULL;

	memset(paiv, '\0', sizeof(struct pai_val));

	paiv->pai_protected = (CVAL(buf,PAI_FLAG_OFFSET) == PAI_ACL_FLAG_PROTECTED);

	paiv->num_entries = SVAL(buf,PAI_NUM_ENTRIES_OFFSET);
	paiv->num_def_entries = SVAL(buf,PAI_NUM_DEFAULT_ENTRIES_OFFSET);

	entry_offset = buf + PAI_ENTRIES_BASE;

	DEBUG(10,("create_pai_val:%s num_entries = %u, num_def_entries = %u\n",
			paiv->pai_protected ? " (pai_protected)" : "", paiv->num_entries, paiv->num_def_entries ));

	for (i = 0; i < paiv->num_entries; i++) {
		struct pai_entry *paie;

		paie = SMB_MALLOC_P(struct pai_entry);
		if (!paie) {
			free_inherited_info(paiv);
			return NULL;
		}

		paie->owner_type = (enum ace_owner)CVAL(entry_offset,0);
		switch( paie->owner_type) {
			case UID_ACE:
				paie->unix_ug.uid = (uid_t)IVAL(entry_offset,1);
				DEBUG(10,("create_pai_val: uid = %u\n", (unsigned int)paie->unix_ug.uid ));
				break;
			case GID_ACE:
				paie->unix_ug.gid = (gid_t)IVAL(entry_offset,1);
				DEBUG(10,("create_pai_val: gid = %u\n", (unsigned int)paie->unix_ug.gid ));
				break;
			case WORLD_ACE:
				paie->unix_ug.world = -1;
				DEBUG(10,("create_pai_val: world ace\n"));
				break;
			default:
				free_inherited_info(paiv);
				return NULL;
		}
		entry_offset += PAI_ENTRY_LENGTH;
		DLIST_ADD(paiv->entry_list, paie);
	}

	for (i = 0; i < paiv->num_def_entries; i++) {
		struct pai_entry *paie;

		paie = SMB_MALLOC_P(struct pai_entry);
		if (!paie) {
			free_inherited_info(paiv);
			return NULL;
		}

		paie->owner_type = (enum ace_owner)CVAL(entry_offset,0);
		switch( paie->owner_type) {
			case UID_ACE:
				paie->unix_ug.uid = (uid_t)IVAL(entry_offset,1);
				DEBUG(10,("create_pai_val: (def) uid = %u\n", (unsigned int)paie->unix_ug.uid ));
				break;
			case GID_ACE:
				paie->unix_ug.gid = (gid_t)IVAL(entry_offset,1);
				DEBUG(10,("create_pai_val: (def) gid = %u\n", (unsigned int)paie->unix_ug.gid ));
				break;
			case WORLD_ACE:
				paie->unix_ug.world = -1;
				DEBUG(10,("create_pai_val: (def) world ace\n"));
				break;
			default:
				free_inherited_info(paiv);
				return NULL;
		}
		entry_offset += PAI_ENTRY_LENGTH;
		DLIST_ADD(paiv->def_entry_list, paie);
	}

	return paiv;
}

/************************************************************************
 Load the user.SAMBA_PAI attribute.
************************************************************************/

static struct pai_val *load_inherited_info(files_struct *fsp)
{
	char *pai_buf;
	size_t pai_buf_size = 1024;
	struct pai_val *paiv = NULL;
	ssize_t ret;

	if (!lp_map_acl_inherit(SNUM(fsp->conn)))
		return NULL;

	if ((pai_buf = SMB_MALLOC(pai_buf_size)) == NULL)
		return NULL;

	do {
		if (fsp->fh->fd != -1)
			ret = SMB_VFS_FGETXATTR(fsp, fsp->fh->fd, SAMBA_POSIX_INHERITANCE_EA_NAME,
					pai_buf, pai_buf_size);
		else
			ret = SMB_VFS_GETXATTR(fsp->conn,fsp->fsp_name,SAMBA_POSIX_INHERITANCE_EA_NAME,
					pai_buf, pai_buf_size);

		if (ret == -1) {
			if (errno != ERANGE) {
				break;
			}
			/* Buffer too small - enlarge it. */
			pai_buf_size *= 2;
			SAFE_FREE(pai_buf);
			if (pai_buf_size > 1024*1024) {
				return NULL; /* Limit malloc to 1mb. */
			}
			if ((pai_buf = SMB_MALLOC(pai_buf_size)) == NULL)
				return NULL;
		}
	} while (ret == -1);

	DEBUG(10,("load_inherited_info: ret = %lu for file %s\n", (unsigned long)ret, fsp->fsp_name));

	if (ret == -1) {
		/* No attribute or not supported. */
#if defined(ENOATTR)
		if (errno != ENOATTR)
			DEBUG(10,("load_inherited_info: Error %s\n", strerror(errno) ));
#else
		if (errno != ENOSYS)
			DEBUG(10,("load_inherited_info: Error %s\n", strerror(errno) ));
#endif
		SAFE_FREE(pai_buf);
		return NULL;
	}

	paiv = create_pai_val(pai_buf, ret);

	if (paiv && paiv->pai_protected)
		DEBUG(10,("load_inherited_info: ACL is protected for file %s\n", fsp->fsp_name));

	SAFE_FREE(pai_buf);
	return paiv;
}

/****************************************************************************
 Functions to manipulate the internal ACE format.
****************************************************************************/

/****************************************************************************
 Count a linked list of canonical ACE entries.
****************************************************************************/

static size_t count_canon_ace_list( canon_ace *list_head )
{
	size_t count = 0;
	canon_ace *ace;

	for (ace = list_head; ace; ace = ace->next)
		count++;

	return count;
}

/****************************************************************************
 Free a linked list of canonical ACE entries.
****************************************************************************/

static void free_canon_ace_list( canon_ace *list_head )
{
	canon_ace *list, *next;

	for (list = list_head; list; list = next) {
		next = list->next;
		DLIST_REMOVE(list_head, list);
		SAFE_FREE(list);
	}
}

/****************************************************************************
 Function to duplicate a canon_ace entry.
****************************************************************************/

static canon_ace *dup_canon_ace( canon_ace *src_ace)
{
	canon_ace *dst_ace = SMB_MALLOC_P(canon_ace);

	if (dst_ace == NULL)
		return NULL;

	*dst_ace = *src_ace;
	dst_ace->prev = dst_ace->next = NULL;
	return dst_ace;
}

/****************************************************************************
 Print out a canon ace.
****************************************************************************/

static void print_canon_ace(canon_ace *pace, int num)
{
	fstring str;

	dbgtext( "canon_ace index %d. Type = %s ", num, pace->attr == ALLOW_ACE ? "allow" : "deny" );
	dbgtext( "SID = %s ", sid_to_string( str, &pace->trustee));
	if (pace->owner_type == UID_ACE) {
		const char *u_name = uidtoname(pace->unix_ug.uid);
		dbgtext( "uid %u (%s) ", (unsigned int)pace->unix_ug.uid, u_name );
	} else if (pace->owner_type == GID_ACE) {
		char *g_name = gidtoname(pace->unix_ug.gid);
		dbgtext( "gid %u (%s) ", (unsigned int)pace->unix_ug.gid, g_name );
	} else
		dbgtext( "other ");
	switch (pace->type) {
		case SMB_ACL_USER:
			dbgtext( "SMB_ACL_USER ");
			break;
		case SMB_ACL_USER_OBJ:
			dbgtext( "SMB_ACL_USER_OBJ ");
			break;
		case SMB_ACL_GROUP:
			dbgtext( "SMB_ACL_GROUP ");
			break;
		case SMB_ACL_GROUP_OBJ:
			dbgtext( "SMB_ACL_GROUP_OBJ ");
			break;
		case SMB_ACL_OTHER:
			dbgtext( "SMB_ACL_OTHER ");
			break;
	}
	if (pace->inherited)
		dbgtext( "(inherited) ");
	dbgtext( "perms ");
	dbgtext( "%c", pace->perms & S_IRUSR ? 'r' : '-');
	dbgtext( "%c", pace->perms & S_IWUSR ? 'w' : '-');
	dbgtext( "%c\n", pace->perms & S_IXUSR ? 'x' : '-');
}

/****************************************************************************
 Print out a canon ace list.
****************************************************************************/

static void print_canon_ace_list(const char *name, canon_ace *ace_list)
{
	int count = 0;

	if( DEBUGLVL( 10 )) {
		dbgtext( "print_canon_ace_list: %s\n", name );
		for (;ace_list; ace_list = ace_list->next, count++)
			print_canon_ace(ace_list, count );
	}
}

/****************************************************************************
 Map POSIX ACL perms to canon_ace permissions (a mode_t containing only S_(R|W|X)USR bits).
****************************************************************************/

static mode_t convert_permset_to_mode_t(connection_struct *conn, SMB_ACL_PERMSET_T permset)
{
	mode_t ret = 0;

	ret |= (SMB_VFS_SYS_ACL_GET_PERM(conn, permset, SMB_ACL_READ) ? S_IRUSR : 0);
	ret |= (SMB_VFS_SYS_ACL_GET_PERM(conn, permset, SMB_ACL_WRITE) ? S_IWUSR : 0);
	ret |= (SMB_VFS_SYS_ACL_GET_PERM(conn, permset, SMB_ACL_EXECUTE) ? S_IXUSR : 0);

	return ret;
}

/****************************************************************************
 Map generic UNIX permissions to canon_ace permissions (a mode_t containing only S_(R|W|X)USR bits).
****************************************************************************/

static mode_t unix_perms_to_acl_perms(mode_t mode, int r_mask, int w_mask, int x_mask)
{
	mode_t ret = 0;

	if (mode & r_mask)
		ret |= S_IRUSR;
	if (mode & w_mask)
		ret |= S_IWUSR;
	if (mode & x_mask)
		ret |= S_IXUSR;

	return ret;
}

/****************************************************************************
 Map canon_ace permissions (a mode_t containing only S_(R|W|X)USR bits) to
 an SMB_ACL_PERMSET_T.
****************************************************************************/

static int map_acl_perms_to_permset(connection_struct *conn, mode_t mode, SMB_ACL_PERMSET_T *p_permset)
{
	if (SMB_VFS_SYS_ACL_CLEAR_PERMS(conn, *p_permset) ==  -1)
		return -1;
	if (mode & S_IRUSR) {
		if (SMB_VFS_SYS_ACL_ADD_PERM(conn, *p_permset, SMB_ACL_READ) == -1)
			return -1;
	}
	if (mode & S_IWUSR) {
		if (SMB_VFS_SYS_ACL_ADD_PERM(conn, *p_permset, SMB_ACL_WRITE) == -1)
			return -1;
	}
	if (mode & S_IXUSR) {
		if (SMB_VFS_SYS_ACL_ADD_PERM(conn, *p_permset, SMB_ACL_EXECUTE) == -1)
			return -1;
	}
	return 0;
}
/****************************************************************************
 Function to create owner and group SIDs from a SMB_STRUCT_STAT.
****************************************************************************/

static void create_file_sids(SMB_STRUCT_STAT *psbuf, DOM_SID *powner_sid, DOM_SID *pgroup_sid)
{
	uid_to_sid( powner_sid, psbuf->st_uid );
	gid_to_sid( pgroup_sid, psbuf->st_gid );
}

/****************************************************************************
 Merge aces with a common sid - if both are allow or deny, OR the permissions together and
 delete the second one. If the first is deny, mask the permissions off and delete the allow
 if the permissions become zero, delete the deny if the permissions are non zero.
****************************************************************************/

static void merge_aces( canon_ace **pp_list_head )
{
	canon_ace *list_head = *pp_list_head;
	canon_ace *curr_ace_outer;
	canon_ace *curr_ace_outer_next;

	/*
	 * First, merge allow entries with identical SIDs, and deny entries
	 * with identical SIDs.
	 */

	for (curr_ace_outer = list_head; curr_ace_outer; curr_ace_outer = curr_ace_outer_next) {
		canon_ace *curr_ace;
		canon_ace *curr_ace_next;

		curr_ace_outer_next = curr_ace_outer->next; /* Save the link in case we delete. */

		for (curr_ace = curr_ace_outer->next; curr_ace; curr_ace = curr_ace_next) {

			curr_ace_next = curr_ace->next; /* Save the link in case of delete. */

			if (sid_equal(&curr_ace->trustee, &curr_ace_outer->trustee) &&
				(curr_ace->attr == curr_ace_outer->attr)) {

				if( DEBUGLVL( 10 )) {
					dbgtext("merge_aces: Merging ACE's\n");
					print_canon_ace( curr_ace_outer, 0);
					print_canon_ace( curr_ace, 0);
				}

				/* Merge two allow or two deny ACE's. */

				curr_ace_outer->perms |= curr_ace->perms;
				DLIST_REMOVE(list_head, curr_ace);
				SAFE_FREE(curr_ace);
				curr_ace_outer_next = curr_ace_outer->next; /* We may have deleted the link. */
			}
		}
	}

	/*
	 * Now go through and mask off allow permissions with deny permissions.
	 * We can delete either the allow or deny here as we know that each SID
	 * appears only once in the list.
	 */

	for (curr_ace_outer = list_head; curr_ace_outer; curr_ace_outer = curr_ace_outer_next) {
		canon_ace *curr_ace;
		canon_ace *curr_ace_next;

		curr_ace_outer_next = curr_ace_outer->next; /* Save the link in case we delete. */

		for (curr_ace = curr_ace_outer->next; curr_ace; curr_ace = curr_ace_next) {

			curr_ace_next = curr_ace->next; /* Save the link in case of delete. */

			/*
			 * Subtract ACE's with different entries. Due to the ordering constraints
			 * we've put on the ACL, we know the deny must be the first one.
			 */

			if (sid_equal(&curr_ace->trustee, &curr_ace_outer->trustee) &&
				(curr_ace_outer->attr == DENY_ACE) && (curr_ace->attr == ALLOW_ACE)) {

				if( DEBUGLVL( 10 )) {
					dbgtext("merge_aces: Masking ACE's\n");
					print_canon_ace( curr_ace_outer, 0);
					print_canon_ace( curr_ace, 0);
				}

				curr_ace->perms &= ~curr_ace_outer->perms;

				if (curr_ace->perms == 0) {

					/*
					 * The deny overrides the allow. Remove the allow.
					 */

					DLIST_REMOVE(list_head, curr_ace);
					SAFE_FREE(curr_ace);
					curr_ace_outer_next = curr_ace_outer->next; /* We may have deleted the link. */

				} else {

					/*
					 * Even after removing permissions, there
					 * are still allow permissions - delete the deny.
					 * It is safe to delete the deny here,
					 * as we are guarenteed by the deny first
					 * ordering that all the deny entries for
					 * this SID have already been merged into one
					 * before we can get to an allow ace.
					 */

					DLIST_REMOVE(list_head, curr_ace_outer);
					SAFE_FREE(curr_ace_outer);
					break;
				}
			}

		} /* end for curr_ace */
	} /* end for curr_ace_outer */

	/* We may have modified the list. */

	*pp_list_head = list_head;
}

/****************************************************************************
 Check if we need to return NT4.x compatible ACL entries.
****************************************************************************/

static BOOL nt4_compatible_acls(void)
{
	int compat = lp_acl_compatibility();

	if (compat == ACL_COMPAT_AUTO) {
		enum remote_arch_types ra_type = get_remote_arch();

		/* Automatically adapt to client */
		return (ra_type <= RA_WINNT);
	} else
		return (compat == ACL_COMPAT_WINNT);
}


/****************************************************************************
 Map canon_ace perms to permission bits NT.
 The attr element is not used here - we only process deny entries on set,
 not get. Deny entries are implicit on get with ace->perms = 0.
****************************************************************************/

static SEC_ACCESS map_canon_ace_perms(int snum, int *pacl_type, DOM_SID *powner_sid, canon_ace *ace, BOOL directory_ace)
{
	SEC_ACCESS sa;
	uint32 nt_mask = 0;

	*pacl_type = SEC_ACE_TYPE_ACCESS_ALLOWED;

	if (lp_acl_map_full_control(snum) && ((ace->perms & ALL_ACE_PERMS) == ALL_ACE_PERMS)) {
		if (directory_ace) {
			nt_mask = UNIX_DIRECTORY_ACCESS_RWX;
		} else {
			nt_mask = UNIX_ACCESS_RWX;
		}
	} else if ((ace->perms & ALL_ACE_PERMS) == (mode_t)0) {
		/*
		 * Windows NT refuses to display ACEs with no permissions in them (but
		 * they are perfectly legal with Windows 2000). If the ACE has empty
		 * permissions we cannot use 0, so we use the otherwise unused
		 * WRITE_OWNER permission, which we ignore when we set an ACL.
		 * We abstract this into a #define of UNIX_ACCESS_NONE to allow this
		 * to be changed in the future.
		 */

		if (nt4_compatible_acls())
			nt_mask = UNIX_ACCESS_NONE;
		else
			nt_mask = 0;
	} else {
		if (directory_ace) {
			nt_mask |= ((ace->perms & S_IRUSR) ? UNIX_DIRECTORY_ACCESS_R : 0 );
			nt_mask |= ((ace->perms & S_IWUSR) ? UNIX_DIRECTORY_ACCESS_W : 0 );
			nt_mask |= ((ace->perms & S_IXUSR) ? UNIX_DIRECTORY_ACCESS_X : 0 );
		} else {
			nt_mask |= ((ace->perms & S_IRUSR) ? UNIX_ACCESS_R : 0 );
			nt_mask |= ((ace->perms & S_IWUSR) ? UNIX_ACCESS_W : 0 );
			nt_mask |= ((ace->perms & S_IXUSR) ? UNIX_ACCESS_X : 0 );
		}
	}

	DEBUG(10,("map_canon_ace_perms: Mapped (UNIX) %x to (NT) %x\n",
			(unsigned int)ace->perms, (unsigned int)nt_mask ));

	init_sec_access(&sa,nt_mask);
	return sa;
}

/****************************************************************************
 Map NT perms to a UNIX mode_t.
****************************************************************************/

#define FILE_SPECIFIC_READ_BITS (FILE_READ_DATA|FILE_READ_EA|FILE_READ_ATTRIBUTES)
#define FILE_SPECIFIC_WRITE_BITS (FILE_WRITE_DATA|FILE_APPEND_DATA|FILE_WRITE_EA|FILE_WRITE_ATTRIBUTES)
#define FILE_SPECIFIC_EXECUTE_BITS (FILE_EXECUTE)

static mode_t map_nt_perms( SEC_ACCESS sec_access, int type)
{
	mode_t mode = 0;

	switch(type) {
	case S_IRUSR:
		if(sec_access.mask & GENERIC_ALL_ACCESS)
			mode = S_IRUSR|S_IWUSR|S_IXUSR;
		else {
			mode |= (sec_access.mask & (GENERIC_READ_ACCESS|FILE_SPECIFIC_READ_BITS)) ? S_IRUSR : 0;
			mode |= (sec_access.mask & (GENERIC_WRITE_ACCESS|FILE_SPECIFIC_WRITE_BITS)) ? S_IWUSR : 0;
			mode |= (sec_access.mask & (GENERIC_EXECUTE_ACCESS|FILE_SPECIFIC_EXECUTE_BITS)) ? S_IXUSR : 0;
		}
		break;
	case S_IRGRP:
		if(sec_access.mask & GENERIC_ALL_ACCESS)
			mode = S_IRGRP|S_IWGRP|S_IXGRP;
		else {
			mode |= (sec_access.mask & (GENERIC_READ_ACCESS|FILE_SPECIFIC_READ_BITS)) ? S_IRGRP : 0;
			mode |= (sec_access.mask & (GENERIC_WRITE_ACCESS|FILE_SPECIFIC_WRITE_BITS)) ? S_IWGRP : 0;
			mode |= (sec_access.mask & (GENERIC_EXECUTE_ACCESS|FILE_SPECIFIC_EXECUTE_BITS)) ? S_IXGRP : 0;
		}
		break;
	case S_IROTH:
		if(sec_access.mask & GENERIC_ALL_ACCESS)
			mode = S_IROTH|S_IWOTH|S_IXOTH;
		else {
			mode |= (sec_access.mask & (GENERIC_READ_ACCESS|FILE_SPECIFIC_READ_BITS)) ? S_IROTH : 0;
			mode |= (sec_access.mask & (GENERIC_WRITE_ACCESS|FILE_SPECIFIC_WRITE_BITS)) ? S_IWOTH : 0;
			mode |= (sec_access.mask & (GENERIC_EXECUTE_ACCESS|FILE_SPECIFIC_EXECUTE_BITS)) ? S_IXOTH : 0;
		}
		break;
	}

	return mode;
}

/****************************************************************************
 Unpack a SEC_DESC into a UNIX owner and group.
****************************************************************************/

static BOOL unpack_nt_owners(int snum, SMB_STRUCT_STAT *psbuf, uid_t *puser, gid_t *pgrp, uint32 security_info_sent, SEC_DESC *psd)
{
	DOM_SID owner_sid;
	DOM_SID grp_sid;

	*puser = (uid_t)-1;
	*pgrp = (gid_t)-1;

	if(security_info_sent == 0) {
		DEBUG(0,("unpack_nt_owners: no security info sent !\n"));
		return True;
	}

	/*
	 * Validate the owner and group SID's.
	 */

	memset(&owner_sid, '\0', sizeof(owner_sid));
	memset(&grp_sid, '\0', sizeof(grp_sid));

	DEBUG(5,("unpack_nt_owners: validating owner_sids.\n"));

	/*
	 * Don't immediately fail if the owner sid cannot be validated.
	 * This may be a group chown only set.
	 */

	if (security_info_sent & OWNER_SECURITY_INFORMATION) {
		sid_copy(&owner_sid, psd->owner_sid);
		if (!sid_to_uid(&owner_sid, puser)) {
			if (lp_force_unknown_acl_user(snum)) {
				/* this allows take ownership to work
				 * reasonably */
				*puser = current_user.ut.uid;
			} else {
				DEBUG(3,("unpack_nt_owners: unable to validate"
					 " owner sid for %s\n",
					 sid_string_static(&owner_sid)));
				return False;
			}
		}
 	}

	/*
	 * Don't immediately fail if the group sid cannot be validated.
	 * This may be an owner chown only set.
	 */

	if (security_info_sent & GROUP_SECURITY_INFORMATION) {
		sid_copy(&grp_sid, psd->grp_sid);
		if (!sid_to_gid( &grp_sid, pgrp)) {
			if (lp_force_unknown_acl_user(snum)) {
				/* this allows take group ownership to work
				 * reasonably */
				*pgrp = current_user.ut.gid;
			} else {
				DEBUG(3,("unpack_nt_owners: unable to validate"
					 " group sid.\n"));
				return False;
			}
		}
	}

	DEBUG(5,("unpack_nt_owners: owner_sids validated.\n"));

	return True;
}

/****************************************************************************
 Ensure the enforced permissions for this share apply.
****************************************************************************/

static void apply_default_perms(files_struct *fsp, canon_ace *pace, mode_t type)
{
	int snum = SNUM(fsp->conn);
	mode_t and_bits = (mode_t)0;
	mode_t or_bits = (mode_t)0;

	/* Get the initial bits to apply. */

	if (fsp->is_directory) {
		and_bits = lp_dir_security_mask(snum);
		or_bits = lp_force_dir_security_mode(snum);
	} else {
		and_bits = lp_security_mask(snum);
		or_bits = lp_force_security_mode(snum);
	}

	/* Now bounce them into the S_USR space. */	
	switch(type) {
	case S_IRUSR:
		/* Ensure owner has read access. */
		pace->perms |= S_IRUSR;
		if (fsp->is_directory)
			pace->perms |= (S_IWUSR|S_IXUSR);
		and_bits = unix_perms_to_acl_perms(and_bits, S_IRUSR, S_IWUSR, S_IXUSR);
		or_bits = unix_perms_to_acl_perms(or_bits, S_IRUSR, S_IWUSR, S_IXUSR);
		break;
	case S_IRGRP:
		and_bits = unix_perms_to_acl_perms(and_bits, S_IRGRP, S_IWGRP, S_IXGRP);
		or_bits = unix_perms_to_acl_perms(or_bits, S_IRGRP, S_IWGRP, S_IXGRP);
		break;
	case S_IROTH:
		and_bits = unix_perms_to_acl_perms(and_bits, S_IROTH, S_IWOTH, S_IXOTH);
		or_bits = unix_perms_to_acl_perms(or_bits, S_IROTH, S_IWOTH, S_IXOTH);
		break;
	}

	pace->perms = ((pace->perms & and_bits)|or_bits);
}

/****************************************************************************
 Check if a given uid/SID is in a group gid/SID. This is probably very
 expensive and will need optimisation. A *lot* of optimisation :-). JRA.
****************************************************************************/

static BOOL uid_entry_in_group( canon_ace *uid_ace, canon_ace *group_ace )
{
	fstring u_name;

	/* "Everyone" always matches every uid. */

	if (sid_equal(&group_ace->trustee, &global_sid_World))
		return True;

	/* Assume that the current user is in the current group (force group) */

	if (uid_ace->unix_ug.uid == current_user.ut.uid && group_ace->unix_ug.gid == current_user.ut.gid)
		return True;

	fstrcpy(u_name, uidtoname(uid_ace->unix_ug.uid));
	return user_in_group_sid(u_name, &group_ace->trustee);
}

/****************************************************************************
 A well formed POSIX file or default ACL has at least 3 entries, a 
 SMB_ACL_USER_OBJ, SMB_ACL_GROUP_OBJ, SMB_ACL_OTHER_OBJ.
 In addition, the owner must always have at least read access.
 When using this call on get_acl, the pst struct is valid and contains
 the mode of the file. When using this call on set_acl, the pst struct has
 been modified to have a mode containing the default for this file or directory
 type.
****************************************************************************/

static BOOL ensure_canon_entry_valid(canon_ace **pp_ace,
							files_struct *fsp,
							const DOM_SID *pfile_owner_sid,
							const DOM_SID *pfile_grp_sid,
							SMB_STRUCT_STAT *pst,
							BOOL setting_acl)
{
	canon_ace *pace;
	BOOL got_user = False;
	BOOL got_grp = False;
	BOOL got_other = False;
	canon_ace *pace_other = NULL;

	for (pace = *pp_ace; pace; pace = pace->next) {
		if (pace->type == SMB_ACL_USER_OBJ) {

			if (setting_acl)
				apply_default_perms(fsp, pace, S_IRUSR);
			got_user = True;

		} else if (pace->type == SMB_ACL_GROUP_OBJ) {

			/*
			 * Ensure create mask/force create mode is respected on set.
			 */

			if (setting_acl)
				apply_default_perms(fsp, pace, S_IRGRP);
			got_grp = True;

		} else if (pace->type == SMB_ACL_OTHER) {

			/*
			 * Ensure create mask/force create mode is respected on set.
			 */

			if (setting_acl)
				apply_default_perms(fsp, pace, S_IROTH);
			got_other = True;
			pace_other = pace;
		}
	}

	if (!got_user) {
		if ((pace = SMB_MALLOC_P(canon_ace)) == NULL) {
			DEBUG(0,("ensure_canon_entry_valid: malloc fail.\n"));
			return False;
		}

		ZERO_STRUCTP(pace);
		pace->type = SMB_ACL_USER_OBJ;
		pace->owner_type = UID_ACE;
		pace->unix_ug.uid = pst->st_uid;
		pace->trustee = *pfile_owner_sid;
		pace->attr = ALLOW_ACE;

		if (setting_acl) {
			/* See if the owning user is in any of the other groups in
			   the ACE. If so, OR in the permissions from that group. */

			BOOL group_matched = False;
			canon_ace *pace_iter;

			for (pace_iter = *pp_ace; pace_iter; pace_iter = pace_iter->next) {
				if (pace_iter->type == SMB_ACL_GROUP_OBJ || pace_iter->type == SMB_ACL_GROUP) {
					if (uid_entry_in_group(pace, pace_iter)) {
						pace->perms |= pace_iter->perms;
						group_matched = True;
					}
				}
			}

			/* If we only got an "everyone" perm, just use that. */
			if (!group_matched) {
				if (got_other)
					pace->perms = pace_other->perms;
				else
					pace->perms = 0;
			}

			apply_default_perms(fsp, pace, S_IRUSR);
		} else {
			pace->perms = unix_perms_to_acl_perms(pst->st_mode, S_IRUSR, S_IWUSR, S_IXUSR);
		}

		DLIST_ADD(*pp_ace, pace);
	}

	if (!got_grp) {
		if ((pace = SMB_MALLOC_P(canon_ace)) == NULL) {
			DEBUG(0,("ensure_canon_entry_valid: malloc fail.\n"));
			return False;
		}

		ZERO_STRUCTP(pace);
		pace->type = SMB_ACL_GROUP_OBJ;
		pace->owner_type = GID_ACE;
		pace->unix_ug.uid = pst->st_gid;
		pace->trustee = *pfile_grp_sid;
		pace->attr = ALLOW_ACE;
		if (setting_acl) {
			/* If we only got an "everyone" perm, just use that. */
			if (got_other)
				pace->perms = pace_other->perms;
			else
				pace->perms = 0;
			apply_default_perms(fsp, pace, S_IRGRP);
		} else {
			pace->perms = unix_perms_to_acl_perms(pst->st_mode, S_IRGRP, S_IWGRP, S_IXGRP);
		}

		DLIST_ADD(*pp_ace, pace);
	}

	if (!got_other) {
		if ((pace = SMB_MALLOC_P(canon_ace)) == NULL) {
			DEBUG(0,("ensure_canon_entry_valid: malloc fail.\n"));
			return False;
		}

		ZERO_STRUCTP(pace);
		pace->type = SMB_ACL_OTHER;
		pace->owner_type = WORLD_ACE;
		pace->unix_ug.world = -1;
		pace->trustee = global_sid_World;
		pace->attr = ALLOW_ACE;
		if (setting_acl) {
			pace->perms = 0;
			apply_default_perms(fsp, pace, S_IROTH);
		} else
			pace->perms = unix_perms_to_acl_perms(pst->st_mode, S_IROTH, S_IWOTH, S_IXOTH);

		DLIST_ADD(*pp_ace, pace);
	}

	return True;
}

/****************************************************************************
 Check if a POSIX ACL has the required SMB_ACL_USER_OBJ and SMB_ACL_GROUP_OBJ entries.
 If it does not have them, check if there are any entries where the trustee is the
 file owner or the owning group, and map these to SMB_ACL_USER_OBJ and SMB_ACL_GROUP_OBJ.
****************************************************************************/

static void check_owning_objs(canon_ace *ace, DOM_SID *pfile_owner_sid, DOM_SID *pfile_grp_sid)
{
	BOOL got_user_obj, got_group_obj;
	canon_ace *current_ace;
	int i, entries;

	entries = count_canon_ace_list(ace);
	got_user_obj = False;
	got_group_obj = False;

	for (i=0, current_ace = ace; i < entries; i++, current_ace = current_ace->next) {
		if (current_ace->type == SMB_ACL_USER_OBJ)
			got_user_obj = True;
		else if (current_ace->type == SMB_ACL_GROUP_OBJ)
			got_group_obj = True;
	}
	if (got_user_obj && got_group_obj) {
		DEBUG(10,("check_owning_objs: ACL had owning user/group entries.\n"));
		return;
	}

	for (i=0, current_ace = ace; i < entries; i++, current_ace = current_ace->next) {
		if (!got_user_obj && current_ace->owner_type == UID_ACE &&
				sid_equal(&current_ace->trustee, pfile_owner_sid)) {
			current_ace->type = SMB_ACL_USER_OBJ;
			got_user_obj = True;
		}
		if (!got_group_obj && current_ace->owner_type == GID_ACE &&
				sid_equal(&current_ace->trustee, pfile_grp_sid)) {
			current_ace->type = SMB_ACL_GROUP_OBJ;
			got_group_obj = True;
		}
	}
	if (!got_user_obj)
		DEBUG(10,("check_owning_objs: ACL is missing an owner entry.\n"));
	if (!got_group_obj)
		DEBUG(10,("check_owning_objs: ACL is missing an owning group entry.\n"));
}

/****************************************************************************
 Unpack a SEC_DESC into two canonical ace lists.
****************************************************************************/

static BOOL create_canon_ace_lists(files_struct *fsp, SMB_STRUCT_STAT *pst,
							DOM_SID *pfile_owner_sid,
							DOM_SID *pfile_grp_sid,
							canon_ace **ppfile_ace, canon_ace **ppdir_ace,
							SEC_ACL *dacl)
{
	BOOL all_aces_are_inherit_only = (fsp->is_directory ? True : False);
	canon_ace *file_ace = NULL;
	canon_ace *dir_ace = NULL;
	canon_ace *tmp_ace = NULL;
	canon_ace *current_ace = NULL;
	BOOL got_dir_allow = False;
	BOOL got_file_allow = False;
	int i, j;

	*ppfile_ace = NULL;
	*ppdir_ace = NULL;

	/*
	 * Convert the incoming ACL into a more regular form.
	 */

	for(i = 0; i < dacl->num_aces; i++) {
		SEC_ACE *psa = &dacl->ace[i];

		if((psa->type != SEC_ACE_TYPE_ACCESS_ALLOWED) && (psa->type != SEC_ACE_TYPE_ACCESS_DENIED)) {
			DEBUG(3,("create_canon_ace_lists: unable to set anything but an ALLOW or DENY ACE.\n"));
			return False;
		}

		if (nt4_compatible_acls()) {
			/*
			 * The security mask may be UNIX_ACCESS_NONE which should map into
			 * no permissions (we overload the WRITE_OWNER bit for this) or it
			 * should be one of the ALL/EXECUTE/READ/WRITE bits. Arrange for this
			 * to be so. Any other bits override the UNIX_ACCESS_NONE bit.
			 */

			/*
			 * Convert GENERIC bits to specific bits.
			 */
 
			se_map_generic(&psa->info.mask, &file_generic_mapping);

			psa->info.mask &= (UNIX_ACCESS_NONE|FILE_ALL_ACCESS);

			if(psa->info.mask != UNIX_ACCESS_NONE)
				psa->info.mask &= ~UNIX_ACCESS_NONE;
		}
	}

	/*
	 * Deal with the fact that NT 4.x re-writes the canonical format
	 * that we return for default ACLs. If a directory ACE is identical
	 * to a inherited directory ACE then NT changes the bits so that the
	 * first ACE is set to OI|IO and the second ACE for this SID is set
	 * to CI. We need to repair this. JRA.
	 */

	for(i = 0; i < dacl->num_aces; i++) {
		SEC_ACE *psa1 = &dacl->ace[i];

		for (j = i + 1; j < dacl->num_aces; j++) {
			SEC_ACE *psa2 = &dacl->ace[j];

			if (psa1->info.mask != psa2->info.mask)
				continue;

			if (!sid_equal(&psa1->trustee, &psa2->trustee))
				continue;

			/*
			 * Ok - permission bits and SIDs are equal.
			 * Check if flags were re-written.
			 */

			if (psa1->flags & SEC_ACE_FLAG_INHERIT_ONLY) {

				psa1->flags |= (psa2->flags & (SEC_ACE_FLAG_CONTAINER_INHERIT|SEC_ACE_FLAG_OBJECT_INHERIT));
				psa2->flags &= ~(SEC_ACE_FLAG_CONTAINER_INHERIT|SEC_ACE_FLAG_OBJECT_INHERIT);
				
			} else if (psa2->flags & SEC_ACE_FLAG_INHERIT_ONLY) {

				psa2->flags |= (psa1->flags & (SEC_ACE_FLAG_CONTAINER_INHERIT|SEC_ACE_FLAG_OBJECT_INHERIT));
				psa1->flags &= ~(SEC_ACE_FLAG_CONTAINER_INHERIT|SEC_ACE_FLAG_OBJECT_INHERIT);
				
			}
		}
	}

	for(i = 0; i < dacl->num_aces; i++) {
		SEC_ACE *psa = &dacl->ace[i];

		/*
		 * Ignore non-mappable SIDs (NT Authority, BUILTIN etc).
		 */

		if (non_mappable_sid(&psa->trustee)) {
			fstring str;
			DEBUG(10,("create_canon_ace_lists: ignoring non-mappable SID %s\n",
				sid_to_string(str, &psa->trustee) ));
			continue;
		}

		/*
		 * Create a cannon_ace entry representing this NT DACL ACE.
		 */

		if ((current_ace = SMB_MALLOC_P(canon_ace)) == NULL) {
			free_canon_ace_list(file_ace);
			free_canon_ace_list(dir_ace);
			DEBUG(0,("create_canon_ace_lists: malloc fail.\n"));
			return False;
		}

		ZERO_STRUCTP(current_ace);

		sid_copy(&current_ace->trustee, &psa->trustee);

		/*
		 * Try and work out if the SID is a user or group
		 * as we need to flag these differently for POSIX.
		 * Note what kind of a POSIX ACL this should map to.
		 */

		if( sid_equal(&current_ace->trustee, &global_sid_World)) {
			current_ace->owner_type = WORLD_ACE;
			current_ace->unix_ug.world = -1;
			current_ace->type = SMB_ACL_OTHER;
		} else if (sid_equal(&current_ace->trustee, &global_sid_Creator_Owner)) {
			current_ace->owner_type = UID_ACE;
			current_ace->unix_ug.uid = pst->st_uid;
			current_ace->type = SMB_ACL_USER_OBJ;

			/*
			 * The Creator Owner entry only specifies inheritable permissions,
			 * never access permissions. WinNT doesn't always set the ACE to
			 *INHERIT_ONLY, though.
			 */

			if (nt4_compatible_acls())
				psa->flags |= SEC_ACE_FLAG_INHERIT_ONLY;
		} else if (sid_equal(&current_ace->trustee, &global_sid_Creator_Group)) {
			current_ace->owner_type = GID_ACE;
			current_ace->unix_ug.gid = pst->st_gid;
			current_ace->type = SMB_ACL_GROUP_OBJ;

			/*
			 * The Creator Group entry only specifies inheritable permissions,
			 * never access permissions. WinNT doesn't always set the ACE to
			 *INHERIT_ONLY, though.
			 */
			if (nt4_compatible_acls())
				psa->flags |= SEC_ACE_FLAG_INHERIT_ONLY;

		} else if (sid_to_uid( &current_ace->trustee, &current_ace->unix_ug.uid)) {
			current_ace->owner_type = UID_ACE;
			current_ace->type = SMB_ACL_USER;
		} else if (sid_to_gid( &current_ace->trustee, &current_ace->unix_ug.gid)) {
			current_ace->owner_type = GID_ACE;
			current_ace->type = SMB_ACL_GROUP;
		} else {
			fstring str;

			free_canon_ace_list(file_ace);
			free_canon_ace_list(dir_ace);
			DEBUG(0,("create_canon_ace_lists: unable to map SID %s to uid or gid.\n",
				sid_to_string(str, &current_ace->trustee) ));
			SAFE_FREE(current_ace);
			return False;
		}

		/*
		 * Map the given NT permissions into a UNIX mode_t containing only
		 * S_I(R|W|X)USR bits.
		 */

		current_ace->perms |= map_nt_perms( psa->info, S_IRUSR);
		current_ace->attr = (psa->type == SEC_ACE_TYPE_ACCESS_ALLOWED) ? ALLOW_ACE : DENY_ACE;
		current_ace->inherited = ((psa->flags & SEC_ACE_FLAG_INHERITED_ACE) ? True : False);

		/*
		 * Now add the created ace to either the file list, the directory
		 * list, or both. We *MUST* preserve the order here (hence we use
		 * DLIST_ADD_END) as NT ACLs are order dependent.
		 */

		if (fsp->is_directory) {

			/*
			 * We can only add to the default POSIX ACE list if the ACE is
			 * designed to be inherited by both files and directories.
			 */

			if ((psa->flags & (SEC_ACE_FLAG_OBJECT_INHERIT|SEC_ACE_FLAG_CONTAINER_INHERIT)) ==
				(SEC_ACE_FLAG_OBJECT_INHERIT|SEC_ACE_FLAG_CONTAINER_INHERIT)) {

				DLIST_ADD_END(dir_ace, current_ace, tmp_ace);

				/*
				 * Note if this was an allow ace. We can't process
				 * any further deny ace's after this.
				 */

				if (current_ace->attr == ALLOW_ACE)
					got_dir_allow = True;

				if ((current_ace->attr == DENY_ACE) && got_dir_allow) {
					DEBUG(0,("create_canon_ace_lists: malformed ACL in inheritable ACL ! \
Deny entry after Allow entry. Failing to set on file %s.\n", fsp->fsp_name ));
					free_canon_ace_list(file_ace);
					free_canon_ace_list(dir_ace);
					return False;
				}	

				if( DEBUGLVL( 10 )) {
					dbgtext("create_canon_ace_lists: adding dir ACL:\n");
					print_canon_ace( current_ace, 0);
				}

				/*
				 * If this is not an inherit only ACE we need to add a duplicate
				 * to the file acl.
				 */

				if (!(psa->flags & SEC_ACE_FLAG_INHERIT_ONLY)) {
					canon_ace *dup_ace = dup_canon_ace(current_ace);

					if (!dup_ace) {
						DEBUG(0,("create_canon_ace_lists: malloc fail !\n"));
						free_canon_ace_list(file_ace);
						free_canon_ace_list(dir_ace);
						return False;
					}

					/*
					 * We must not free current_ace here as its
					 * pointer is now owned by the dir_ace list.
					 */
					current_ace = dup_ace;
				} else {
					/*
					 * We must not free current_ace here as its
					 * pointer is now owned by the dir_ace list.
					 */
					current_ace = NULL;
				}
			}
		}

		/*
		 * Only add to the file ACL if not inherit only.
		 */

		if (current_ace && !(psa->flags & SEC_ACE_FLAG_INHERIT_ONLY)) {
			DLIST_ADD_END(file_ace, current_ace, tmp_ace);

			/*
			 * Note if this was an allow ace. We can't process
			 * any further deny ace's after this.
			 */

			if (current_ace->attr == ALLOW_ACE)
				got_file_allow = True;

			if ((current_ace->attr == DENY_ACE) && got_file_allow) {
				DEBUG(0,("create_canon_ace_lists: malformed ACL in file ACL ! \
Deny entry after Allow entry. Failing to set on file %s.\n", fsp->fsp_name ));
				free_canon_ace_list(file_ace);
				free_canon_ace_list(dir_ace);
				return False;
			}	

			if( DEBUGLVL( 10 )) {
				dbgtext("create_canon_ace_lists: adding file ACL:\n");
				print_canon_ace( current_ace, 0);
			}
			all_aces_are_inherit_only = False;
			/*
			 * We must not free current_ace here as its
			 * pointer is now owned by the file_ace list.
			 */
			current_ace = NULL;
		}

		/*
		 * Free if ACE was not added.
		 */

		SAFE_FREE(current_ace);
	}

	if (fsp->is_directory && all_aces_are_inherit_only) {
		/*
		 * Windows 2000 is doing one of these weird 'inherit acl'
		 * traverses to conserve NTFS ACL resources. Just pretend
		 * there was no DACL sent. JRA.
		 */

		DEBUG(10,("create_canon_ace_lists: Win2k inherit acl traverse. Ignoring DACL.\n"));
		free_canon_ace_list(file_ace);
		free_canon_ace_list(dir_ace);
		file_ace = NULL;
		dir_ace = NULL;
	} else {
		/*
		 * Check if we have SMB_ACL_USER_OBJ and SMB_ACL_GROUP_OBJ entries in each
		 * ACL. If we don't have them, check if any SMB_ACL_USER/SMB_ACL_GROUP
		 * entries can be converted to *_OBJ. Usually we will already have these
		 * entries in the Default ACL, and the Access ACL will not have them.
		 */
		if (file_ace) {
			check_owning_objs(file_ace, pfile_owner_sid, pfile_grp_sid);
		}
		if (dir_ace) {
			check_owning_objs(dir_ace, pfile_owner_sid, pfile_grp_sid);
		}
	}

	*ppfile_ace = file_ace;
	*ppdir_ace = dir_ace;

	return True;
}

/****************************************************************************
 ASCII art time again... JRA :-).

 We have 4 cases to process when moving from an NT ACL to a POSIX ACL. Firstly,
 we insist the ACL is in canonical form (ie. all DENY entries preceede ALLOW
 entries). Secondly, the merge code has ensured that all duplicate SID entries for
 allow or deny have been merged, so the same SID can only appear once in the deny
 list or once in the allow list.

 We then process as follows :

 ---------------------------------------------------------------------------
 First pass - look for a Everyone DENY entry.

 If it is deny all (rwx) trunate the list at this point.
 Else, walk the list from this point and use the deny permissions of this
 entry as a mask on all following allow entries. Finally, delete
 the Everyone DENY entry (we have applied it to everything possible).

 In addition, in this pass we remove any DENY entries that have 
 no permissions (ie. they are a DENY nothing).
 ---------------------------------------------------------------------------
 Second pass - only deal with deny user entries.

 DENY user1 (perms XXX)

 new_perms = 0
 for all following allow group entries where user1 is in group
	new_perms |= group_perms;

 user1 entry perms = new_perms & ~ XXX;

 Convert the deny entry to an allow entry with the new perms and
 push to the end of the list. Note if the user was in no groups
 this maps to a specific allow nothing entry for this user.

 The common case from the NT ACL choser (userX deny all) is
 optimised so we don't do the group lookup - we just map to
 an allow nothing entry.

 What we're doing here is inferring the allow permissions the
 person setting the ACE on user1 wanted by looking at the allow
 permissions on the groups the user is currently in. This will
 be a snapshot, depending on group membership but is the best
 we can do and has the advantage of failing closed rather than
 open.
 ---------------------------------------------------------------------------
 Third pass - only deal with deny group entries.

 DENY group1 (perms XXX)

 for all following allow user entries where user is in group1
   user entry perms = user entry perms & ~ XXX;

 If there is a group Everyone allow entry with permissions YYY,
 convert the group1 entry to an allow entry and modify its
 permissions to be :

 new_perms = YYY & ~ XXX

 and push to the end of the list.

 If there is no group Everyone allow entry then convert the
 group1 entry to a allow nothing entry and push to the end of the list.

 Note that the common case from the NT ACL choser (groupX deny all)
 cannot be optimised here as we need to modify user entries who are
 in the group to change them to a deny all also.

 What we're doing here is modifying the allow permissions of
 user entries (which are more specific in POSIX ACLs) to mask
 out the explicit deny set on the group they are in. This will
 be a snapshot depending on current group membership but is the
 best we can do and has the advantage of failing closed rather
 than open.
 ---------------------------------------------------------------------------
 Fourth pass - cope with cumulative permissions.

 for all allow user entries, if there exists an allow group entry with
 more permissive permissions, and the user is in that group, rewrite the
 allow user permissions to contain both sets of permissions.

 Currently the code for this is #ifdef'ed out as these semantics make
 no sense to me. JRA.
 ---------------------------------------------------------------------------

 Note we *MUST* do the deny user pass first as this will convert deny user
 entries into allow user entries which can then be processed by the deny
 group pass.

 The above algorithm took a *lot* of thinking about - hence this
 explaination :-). JRA.
****************************************************************************/

/****************************************************************************
 Process a canon_ace list entries. This is very complex code. We need
 to go through and remove the "deny" permissions from any allow entry that matches
 the id of this entry. We have already refused any NT ACL that wasn't in correct
 order (DENY followed by ALLOW). If any allow entry ends up with zero permissions,
 we just remove it (to fail safe). We have already removed any duplicate ace
 entries. Treat an "Everyone" DENY_ACE as a special case - use it to mask all
 allow entries.
****************************************************************************/

static void process_deny_list( canon_ace **pp_ace_list )
{
	canon_ace *ace_list = *pp_ace_list;
	canon_ace *curr_ace = NULL;
	canon_ace *curr_ace_next = NULL;

	/* Pass 1 above - look for an Everyone, deny entry. */

	for (curr_ace = ace_list; curr_ace; curr_ace = curr_ace_next) {
		canon_ace *allow_ace_p;

		curr_ace_next = curr_ace->next; /* So we can't lose the link. */

		if (curr_ace->attr != DENY_ACE)
			continue;

		if (curr_ace->perms == (mode_t)0) {

			/* Deny nothing entry - delete. */

			DLIST_REMOVE(ace_list, curr_ace);
			continue;
		}

		if (!sid_equal(&curr_ace->trustee, &global_sid_World))
			continue;

		/* JRATEST - assert. */
		SMB_ASSERT(curr_ace->owner_type == WORLD_ACE);

		if (curr_ace->perms == ALL_ACE_PERMS) {

			/*
			 * Optimisation. This is a DENY_ALL to Everyone. Truncate the
			 * list at this point including this entry.
			 */

			canon_ace *prev_entry = curr_ace->prev;

			free_canon_ace_list( curr_ace );
			if (prev_entry)
				prev_entry->next = NULL;
			else {
				/* We deleted the entire list. */
				ace_list = NULL;
			}
			break;
		}

		for (allow_ace_p = curr_ace->next; allow_ace_p; allow_ace_p = allow_ace_p->next) {

			/* 
			 * Only mask off allow entries.
			 */

			if (allow_ace_p->attr != ALLOW_ACE)
				continue;

			allow_ace_p->perms &= ~curr_ace->perms;
		}

		/*
		 * Now it's been applied, remove it.
		 */

		DLIST_REMOVE(ace_list, curr_ace);
	}

	/* Pass 2 above - deal with deny user entries. */

	for (curr_ace = ace_list; curr_ace; curr_ace = curr_ace_next) {
		mode_t new_perms = (mode_t)0;
		canon_ace *allow_ace_p;
		canon_ace *tmp_ace;

		curr_ace_next = curr_ace->next; /* So we can't lose the link. */

		if (curr_ace->attr != DENY_ACE)
			continue;

		if (curr_ace->owner_type != UID_ACE)
			continue;

		if (curr_ace->perms == ALL_ACE_PERMS) {

			/*
			 * Optimisation - this is a deny everything to this user.
			 * Convert to an allow nothing and push to the end of the list.
			 */

			curr_ace->attr = ALLOW_ACE;
			curr_ace->perms = (mode_t)0;
			DLIST_DEMOTE(ace_list, curr_ace, tmp_ace);
			continue;
		}

		for (allow_ace_p = curr_ace->next; allow_ace_p; allow_ace_p = allow_ace_p->next) {

			if (allow_ace_p->attr != ALLOW_ACE)
				continue;

			/* We process GID_ACE and WORLD_ACE entries only. */

			if (allow_ace_p->owner_type == UID_ACE)
				continue;

			if (uid_entry_in_group( curr_ace, allow_ace_p))
				new_perms |= allow_ace_p->perms;
		}

		/*
		 * Convert to a allow entry, modify the perms and push to the end
		 * of the list.
		 */

		curr_ace->attr = ALLOW_ACE;
		curr_ace->perms = (new_perms & ~curr_ace->perms);
		DLIST_DEMOTE(ace_list, curr_ace, tmp_ace);
	}

	/* Pass 3 above - deal with deny group entries. */

	for (curr_ace = ace_list; curr_ace; curr_ace = curr_ace_next) {
		canon_ace *tmp_ace;
		canon_ace *allow_ace_p;
		canon_ace *allow_everyone_p = NULL;

		curr_ace_next = curr_ace->next; /* So we can't lose the link. */

		if (curr_ace->attr != DENY_ACE)
			continue;

		if (curr_ace->owner_type != GID_ACE)
			continue;

		for (allow_ace_p = curr_ace->next; allow_ace_p; allow_ace_p = allow_ace_p->next) {

			if (allow_ace_p->attr != ALLOW_ACE)
				continue;

			/* Store a pointer to the Everyone allow, if it exists. */
			if (allow_ace_p->owner_type == WORLD_ACE)
				allow_everyone_p = allow_ace_p;

			/* We process UID_ACE entries only. */

			if (allow_ace_p->owner_type != UID_ACE)
				continue;

			/* Mask off the deny group perms. */

			if (uid_entry_in_group( allow_ace_p, curr_ace))
				allow_ace_p->perms &= ~curr_ace->perms;
		}

		/*
		 * Convert the deny to an allow with the correct perms and
		 * push to the end of the list.
		 */

		curr_ace->attr = ALLOW_ACE;
		if (allow_everyone_p)
			curr_ace->perms = allow_everyone_p->perms & ~curr_ace->perms;
		else
			curr_ace->perms = (mode_t)0;
		DLIST_DEMOTE(ace_list, curr_ace, tmp_ace);

	}

	/* Doing this fourth pass allows Windows semantics to be layered
	 * on top of POSIX semantics. I'm not sure if this is desirable.
	 * For example, in W2K ACLs there is no way to say, "Group X no
	 * access, user Y full access" if user Y is a member of group X.
	 * This seems completely broken semantics to me.... JRA.
	 */

#if 0
	/* Pass 4 above - deal with allow entries. */

	for (curr_ace = ace_list; curr_ace; curr_ace = curr_ace_next) {
		canon_ace *allow_ace_p;

		curr_ace_next = curr_ace->next; /* So we can't lose the link. */

		if (curr_ace->attr != ALLOW_ACE)
			continue;

		if (curr_ace->owner_type != UID_ACE)
			continue;

		for (allow_ace_p = ace_list; allow_ace_p; allow_ace_p = allow_ace_p->next) {

			if (allow_ace_p->attr != ALLOW_ACE)
				continue;

			/* We process GID_ACE entries only. */

			if (allow_ace_p->owner_type != GID_ACE)
				continue;

			/* OR in the group perms. */

			if (uid_entry_in_group( curr_ace, allow_ace_p))
				curr_ace->perms |= allow_ace_p->perms;
		}
	}
#endif

	*pp_ace_list = ace_list;
}

/****************************************************************************
 Create a default mode that will be used if a security descriptor entry has
 no user/group/world entries.
****************************************************************************/

static mode_t create_default_mode(files_struct *fsp, BOOL interitable_mode)
{
	int snum = SNUM(fsp->conn);
	mode_t and_bits = (mode_t)0;
	mode_t or_bits = (mode_t)0;
	mode_t mode = interitable_mode ? unix_mode( fsp->conn, FILE_ATTRIBUTE_ARCHIVE, fsp->fsp_name, False) : S_IRUSR;

	if (fsp->is_directory)
		mode |= (S_IWUSR|S_IXUSR);

	/*
	 * Now AND with the create mode/directory mode bits then OR with the
	 * force create mode/force directory mode bits.
	 */

	if (fsp->is_directory) {
		and_bits = lp_dir_security_mask(snum);
		or_bits = lp_force_dir_security_mode(snum);
	} else {
		and_bits = lp_security_mask(snum);
		or_bits = lp_force_security_mode(snum);
	}

	return ((mode & and_bits)|or_bits);
}

/****************************************************************************
 Unpack a SEC_DESC into two canonical ace lists. We don't depend on this
 succeeding.
****************************************************************************/

static BOOL unpack_canon_ace(files_struct *fsp, 
							SMB_STRUCT_STAT *pst,
							DOM_SID *pfile_owner_sid,
							DOM_SID *pfile_grp_sid,
							canon_ace **ppfile_ace, canon_ace **ppdir_ace,
							uint32 security_info_sent, SEC_DESC *psd)
{
	canon_ace *file_ace = NULL;
	canon_ace *dir_ace = NULL;

	*ppfile_ace = NULL;
	*ppdir_ace = NULL;

	if(security_info_sent == 0) {
		DEBUG(0,("unpack_canon_ace: no security info sent !\n"));
		return False;
	}

	/*
	 * If no DACL then this is a chown only security descriptor.
	 */

	if(!(security_info_sent & DACL_SECURITY_INFORMATION) || !psd->dacl)
		return True;

	/*
	 * Now go through the DACL and create the canon_ace lists.
	 */

	if (!create_canon_ace_lists( fsp, pst, pfile_owner_sid, pfile_grp_sid,
								&file_ace, &dir_ace, psd->dacl))
		return False;

	if ((file_ace == NULL) && (dir_ace == NULL)) {
		/* W2K traverse DACL set - ignore. */
		return True;
	}

	/*
	 * Go through the canon_ace list and merge entries
	 * belonging to identical users of identical allow or deny type.
	 * We can do this as all deny entries come first, followed by
	 * all allow entries (we have mandated this before accepting this acl).
	 */

	print_canon_ace_list( "file ace - before merge", file_ace);
	merge_aces( &file_ace );

	print_canon_ace_list( "dir ace - before merge", dir_ace);
	merge_aces( &dir_ace );

	/*
	 * NT ACLs are order dependent. Go through the acl lists and
	 * process DENY entries by masking the allow entries.
	 */

	print_canon_ace_list( "file ace - before deny", file_ace);
	process_deny_list( &file_ace);

	print_canon_ace_list( "dir ace - before deny", dir_ace);
	process_deny_list( &dir_ace);

	/*
	 * A well formed POSIX file or default ACL has at least 3 entries, a 
	 * SMB_ACL_USER_OBJ, SMB_ACL_GROUP_OBJ, SMB_ACL_OTHER_OBJ
	 * and optionally a mask entry. Ensure this is the case.
	 */

	print_canon_ace_list( "file ace - before valid", file_ace);

	/*
	 * A default 3 element mode entry for a file should be r-- --- ---.
	 * A default 3 element mode entry for a directory should be rwx --- ---.
	 */

	pst->st_mode = create_default_mode(fsp, False);

	if (!ensure_canon_entry_valid(&file_ace, fsp, pfile_owner_sid, pfile_grp_sid, pst, True)) {
		free_canon_ace_list(file_ace);
		free_canon_ace_list(dir_ace);
		return False;
	}

	print_canon_ace_list( "dir ace - before valid", dir_ace);

	/*
	 * A default inheritable 3 element mode entry for a directory should be the
	 * mode Samba will use to create a file within. Ensure user rwx bits are set if
	 * it's a directory.
	 */

	pst->st_mode = create_default_mode(fsp, True);

	if (dir_ace && !ensure_canon_entry_valid(&dir_ace, fsp, pfile_owner_sid, pfile_grp_sid, pst, True)) {
		free_canon_ace_list(file_ace);
		free_canon_ace_list(dir_ace);
		return False;
	}

	print_canon_ace_list( "file ace - return", file_ace);
	print_canon_ace_list( "dir ace - return", dir_ace);

	*ppfile_ace = file_ace;
	*ppdir_ace = dir_ace;
	return True;

}

/******************************************************************************
 When returning permissions, try and fit NT display
 semantics if possible. Note the the canon_entries here must have been malloced.
 The list format should be - first entry = owner, followed by group and other user
 entries, last entry = other.

 Note that this doesn't exactly match the NT semantics for an ACL. As POSIX entries
 are not ordered, and match on the most specific entry rather than walking a list,
 then a simple POSIX permission of rw-r--r-- should really map to 5 entries,

 Entry 0: owner : deny all except read and write.
 Entry 1: owner : allow read and write.
 Entry 2: group : deny all except read.
 Entry 3: group : allow read.
 Entry 4: Everyone : allow read.

 But NT cannot display this in their ACL editor !
********************************************************************************/

static void arrange_posix_perms( char *filename, canon_ace **pp_list_head)
{
	canon_ace *list_head = *pp_list_head;
	canon_ace *owner_ace = NULL;
	canon_ace *other_ace = NULL;
	canon_ace *ace = NULL;

	for (ace = list_head; ace; ace = ace->next) {
		if (ace->type == SMB_ACL_USER_OBJ)
			owner_ace = ace;
		else if (ace->type == SMB_ACL_OTHER) {
			/* Last ace - this is "other" */
			other_ace = ace;
		}
	}
		
	if (!owner_ace || !other_ace) {
		DEBUG(0,("arrange_posix_perms: Invalid POSIX permissions for file %s, missing owner or other.\n",
			filename ));
		return;
	}

	/*
	 * The POSIX algorithm applies to owner first, and other last,
	 * so ensure they are arranged in this order.
	 */

	if (owner_ace) {
		DLIST_PROMOTE(list_head, owner_ace);
	}

	if (other_ace) {
		DLIST_DEMOTE(list_head, other_ace, ace);
	}

	/* We have probably changed the head of the list. */

	*pp_list_head = list_head;
}
		
/****************************************************************************
 Create a linked list of canonical ACE entries.
****************************************************************************/

static canon_ace *canonicalise_acl( files_struct *fsp, SMB_ACL_T posix_acl, SMB_STRUCT_STAT *psbuf,
					const DOM_SID *powner, const DOM_SID *pgroup, struct pai_val *pal, SMB_ACL_TYPE_T the_acl_type)
{
	connection_struct *conn = fsp->conn;
	mode_t acl_mask = (S_IRUSR|S_IWUSR|S_IXUSR);
	canon_ace *list_head = NULL;
	canon_ace *ace = NULL;
	canon_ace *next_ace = NULL;
	int entry_id = SMB_ACL_FIRST_ENTRY;
	SMB_ACL_ENTRY_T entry;
	size_t ace_count;

	while ( posix_acl && (SMB_VFS_SYS_ACL_GET_ENTRY(conn, posix_acl, entry_id, &entry) == 1)) {
		SMB_ACL_TAG_T tagtype;
		SMB_ACL_PERMSET_T permset;
		DOM_SID sid;
		posix_id unix_ug;
		enum ace_owner owner_type;

		/* get_next... */
		if (entry_id == SMB_ACL_FIRST_ENTRY)
			entry_id = SMB_ACL_NEXT_ENTRY;

		/* Is this a MASK entry ? */
		if (SMB_VFS_SYS_ACL_GET_TAG_TYPE(conn, entry, &tagtype) == -1)
			continue;

		if (SMB_VFS_SYS_ACL_GET_PERMSET(conn, entry, &permset) == -1)
			continue;

		/* Decide which SID to use based on the ACL type. */
		switch(tagtype) {
			case SMB_ACL_USER_OBJ:
				/* Get the SID from the owner. */
				sid_copy(&sid, powner);
				unix_ug.uid = psbuf->st_uid;
				owner_type = UID_ACE;
				break;
			case SMB_ACL_USER:
				{
					uid_t *puid = (uid_t *)SMB_VFS_SYS_ACL_GET_QUALIFIER(conn, entry);
					if (puid == NULL) {
						DEBUG(0,("canonicalise_acl: Failed to get uid.\n"));
						continue;
					}
					/*
					 * A SMB_ACL_USER entry for the owner is shadowed by the
					 * SMB_ACL_USER_OBJ entry and Windows also cannot represent
					 * that entry, so we ignore it. We also don't create such
					 * entries out of the blue when setting ACLs, so a get/set
					 * cycle will drop them.
					 */
					if (the_acl_type == SMB_ACL_TYPE_ACCESS && *puid == psbuf->st_uid) {
						SMB_VFS_SYS_ACL_FREE_QUALIFIER(conn, (void *)puid,tagtype);
						continue;
					}
					uid_to_sid( &sid, *puid);
					unix_ug.uid = *puid;
					owner_type = UID_ACE;
					SMB_VFS_SYS_ACL_FREE_QUALIFIER(conn, (void *)puid,tagtype);
					break;
				}
			case SMB_ACL_GROUP_OBJ:
				/* Get the SID from the owning group. */
				sid_copy(&sid, pgroup);
				unix_ug.gid = psbuf->st_gid;
				owner_type = GID_ACE;
				break;
			case SMB_ACL_GROUP:
				{
					gid_t *pgid = (gid_t *)SMB_VFS_SYS_ACL_GET_QUALIFIER(conn, entry);
					if (pgid == NULL) {
						DEBUG(0,("canonicalise_acl: Failed to get gid.\n"));
						continue;
					}
					gid_to_sid( &sid, *pgid);
					unix_ug.gid = *pgid;
					owner_type = GID_ACE;
					SMB_VFS_SYS_ACL_FREE_QUALIFIER(conn, (void *)pgid,tagtype);
					break;
				}
			case SMB_ACL_MASK:
				acl_mask = convert_permset_to_mode_t(conn, permset);
				continue; /* Don't count the mask as an entry. */
			case SMB_ACL_OTHER:
				/* Use the Everyone SID */
				sid = global_sid_World;
				unix_ug.world = -1;
				owner_type = WORLD_ACE;
				break;
			default:
				DEBUG(0,("canonicalise_acl: Unknown tagtype %u\n", (unsigned int)tagtype));
				continue;
		}

		/*
		 * Add this entry to the list.
		 */

		if ((ace = SMB_MALLOC_P(canon_ace)) == NULL)
			goto fail;

		ZERO_STRUCTP(ace);
		ace->type = tagtype;
		ace->perms = convert_permset_to_mode_t(conn, permset);
		ace->attr = ALLOW_ACE;
		ace->trustee = sid;
		ace->unix_ug = unix_ug;
		ace->owner_type = owner_type;
		ace->inherited = get_inherited_flag(pal, ace, (the_acl_type == SMB_ACL_TYPE_DEFAULT));

		DLIST_ADD(list_head, ace);
	}

	/*
	 * This next call will ensure we have at least a user/group/world set.
	 */

	if (!ensure_canon_entry_valid(&list_head, fsp, powner, pgroup, psbuf, False))
		goto fail;

	/*
	 * Now go through the list, masking the permissions with the
	 * acl_mask. Ensure all DENY Entries are at the start of the list.
	 */

	DEBUG(10,("canonicalise_acl: %s ace entries before arrange :\n", the_acl_type == SMB_ACL_TYPE_ACCESS ? "Access" : "Default" ));

	for ( ace_count = 0, ace = list_head; ace; ace = next_ace, ace_count++) {
		next_ace = ace->next;

		/* Masks are only applied to entries other than USER_OBJ and OTHER. */
		if (ace->type != SMB_ACL_OTHER && ace->type != SMB_ACL_USER_OBJ)
			ace->perms &= acl_mask;

		if (ace->perms == 0) {
			DLIST_PROMOTE(list_head, ace);
		}

		if( DEBUGLVL( 10 ) ) {
			print_canon_ace(ace, ace_count);
		}
	}

	arrange_posix_perms(fsp->fsp_name,&list_head );

	print_canon_ace_list( "canonicalise_acl: ace entries after arrange", list_head );

	return list_head;

  fail:

	free_canon_ace_list(list_head);
	return NULL;
}

/****************************************************************************
 Check if the current user group list contains a given group.
****************************************************************************/

static BOOL current_user_in_group(gid_t gid)
{
	int i;

	for (i = 0; i < current_user.ut.ngroups; i++) {
		if (current_user.ut.groups[i] == gid) {
			return True;
		}
	}

	return False;
}

/****************************************************************************
 Should we override a deny ?  Check deprecated 'acl group control'
 and 'dos filemode'
****************************************************************************/

static BOOL acl_group_override(connection_struct *conn, gid_t prim_gid)
{
	if ( (errno == EACCES || errno == EPERM) 
		&& (lp_acl_group_control(SNUM(conn)) || lp_dos_filemode(SNUM(conn)))
		&& current_user_in_group(prim_gid)) 
	{
		return True;
	} 

	return False;
}

/****************************************************************************
 Attempt to apply an ACL to a file or directory.
****************************************************************************/

static BOOL set_canon_ace_list(files_struct *fsp, canon_ace *the_ace, BOOL default_ace, gid_t prim_gid, BOOL *pacl_set_support)
{
	connection_struct *conn = fsp->conn;
	BOOL ret = False;
	SMB_ACL_T the_acl = SMB_VFS_SYS_ACL_INIT(conn, (int)count_canon_ace_list(the_ace) + 1);
	canon_ace *p_ace;
	int i;
	SMB_ACL_ENTRY_T mask_entry;
	BOOL got_mask_entry = False;
	SMB_ACL_PERMSET_T mask_permset;
	SMB_ACL_TYPE_T the_acl_type = (default_ace ? SMB_ACL_TYPE_DEFAULT : SMB_ACL_TYPE_ACCESS);
	BOOL needs_mask = False;
	mode_t mask_perms = 0;

#if defined(POSIX_ACL_NEEDS_MASK)
	/* HP-UX always wants to have a mask (called "class" there). */
	needs_mask = True;
#endif

	if (the_acl == NULL) {

		if (!no_acl_syscall_error(errno)) {
			/*
			 * Only print this error message if we have some kind of ACL
			 * support that's not working. Otherwise we would always get this.
			 */
			DEBUG(0,("set_canon_ace_list: Unable to init %s ACL. (%s)\n",
				default_ace ? "default" : "file", strerror(errno) ));
		}
		*pacl_set_support = False;
		return False;
	}

	if( DEBUGLVL( 10 )) {
		dbgtext("set_canon_ace_list: setting ACL:\n");
		for (i = 0, p_ace = the_ace; p_ace; p_ace = p_ace->next, i++ ) {
			print_canon_ace( p_ace, i);
		}
	}

	for (i = 0, p_ace = the_ace; p_ace; p_ace = p_ace->next, i++ ) {
		SMB_ACL_ENTRY_T the_entry;
		SMB_ACL_PERMSET_T the_permset;

		/*
		 * ACLs only "need" an ACL_MASK entry if there are any named user or
		 * named group entries. But if there is an ACL_MASK entry, it applies
		 * to ACL_USER, ACL_GROUP, and ACL_GROUP_OBJ entries. Set the mask
		 * so that it doesn't deny (i.e., mask off) any permissions.
		 */

		if (p_ace->type == SMB_ACL_USER || p_ace->type == SMB_ACL_GROUP) {
			needs_mask = True;
			mask_perms |= p_ace->perms;
		} else if (p_ace->type == SMB_ACL_GROUP_OBJ) {
			mask_perms |= p_ace->perms;
		}

		/*
		 * Get the entry for this ACE.
		 */

		if (SMB_VFS_SYS_ACL_CREATE_ENTRY(conn, &the_acl, &the_entry) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to create entry %d. (%s)\n",
				i, strerror(errno) ));
			goto fail;
		}

		if (p_ace->type == SMB_ACL_MASK) {
			mask_entry = the_entry;
			got_mask_entry = True;
		}

		/*
		 * Ok - we now know the ACL calls should be working, don't
		 * allow fallback to chmod.
		 */

		*pacl_set_support = True;

		/*
		 * Initialise the entry from the canon_ace.
		 */

		/*
		 * First tell the entry what type of ACE this is.
		 */

		if (SMB_VFS_SYS_ACL_SET_TAG_TYPE(conn, the_entry, p_ace->type) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to set tag type on entry %d. (%s)\n",
				i, strerror(errno) ));
			goto fail;
		}

		/*
		 * Only set the qualifier (user or group id) if the entry is a user
		 * or group id ACE.
		 */

		if ((p_ace->type == SMB_ACL_USER) || (p_ace->type == SMB_ACL_GROUP)) {
			if (SMB_VFS_SYS_ACL_SET_QUALIFIER(conn, the_entry,(void *)&p_ace->unix_ug.uid) == -1) {
				DEBUG(0,("set_canon_ace_list: Failed to set qualifier on entry %d. (%s)\n",
					i, strerror(errno) ));
				goto fail;
			}
		}

		/*
		 * Convert the mode_t perms in the canon_ace to a POSIX permset.
		 */

		if (SMB_VFS_SYS_ACL_GET_PERMSET(conn, the_entry, &the_permset) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to get permset on entry %d. (%s)\n",
				i, strerror(errno) ));
			goto fail;
		}

		if (map_acl_perms_to_permset(conn, p_ace->perms, &the_permset) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to create permset for mode (%u) on entry %d. (%s)\n",
				(unsigned int)p_ace->perms, i, strerror(errno) ));
			goto fail;
		}

		/*
		 * ..and apply them to the entry.
		 */

		if (SMB_VFS_SYS_ACL_SET_PERMSET(conn, the_entry, the_permset) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to add permset on entry %d. (%s)\n",
				i, strerror(errno) ));
			goto fail;
		}

		if( DEBUGLVL( 10 ))
			print_canon_ace( p_ace, i);

	}

	if (needs_mask && !got_mask_entry) {
		if (SMB_VFS_SYS_ACL_CREATE_ENTRY(conn, &the_acl, &mask_entry) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to create mask entry. (%s)\n", strerror(errno) ));
			goto fail;
		}

		if (SMB_VFS_SYS_ACL_SET_TAG_TYPE(conn, mask_entry, SMB_ACL_MASK) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to set tag type on mask entry. (%s)\n",strerror(errno) ));
			goto fail;
		}

		if (SMB_VFS_SYS_ACL_GET_PERMSET(conn, mask_entry, &mask_permset) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to get mask permset. (%s)\n", strerror(errno) ));
			goto fail;
		}

		if (map_acl_perms_to_permset(conn, S_IRUSR|S_IWUSR|S_IXUSR, &mask_permset) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to create mask permset. (%s)\n", strerror(errno) ));
			goto fail;
		}

		if (SMB_VFS_SYS_ACL_SET_PERMSET(conn, mask_entry, mask_permset) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to add mask permset. (%s)\n", strerror(errno) ));
			goto fail;
		}
	}

	/*
	 * Check if the ACL is valid.
	 */

	if (SMB_VFS_SYS_ACL_VALID(conn, the_acl) == -1) {
		DEBUG(0,("set_canon_ace_list: ACL type (%s) is invalid for set (%s).\n",
				the_acl_type == SMB_ACL_TYPE_DEFAULT ? "directory default" : "file",
				strerror(errno) ));
		goto fail;
	}

	/*
	 * Finally apply it to the file or directory.
	 */

	if(default_ace || fsp->is_directory || fsp->fh->fd == -1) {
		if (SMB_VFS_SYS_ACL_SET_FILE(conn, fsp->fsp_name, the_acl_type, the_acl) == -1) {
			/*
			 * Some systems allow all the above calls and only fail with no ACL support
			 * when attempting to apply the acl. HPUX with HFS is an example of this. JRA.
			 */
			if (no_acl_syscall_error(errno)) {
				*pacl_set_support = False;
			}

			if (acl_group_override(conn, prim_gid)) {
				int sret;

				DEBUG(5,("set_canon_ace_list: acl group control on and current user in file %s primary group.\n",
					fsp->fsp_name ));

				become_root();
				sret = SMB_VFS_SYS_ACL_SET_FILE(conn, fsp->fsp_name, the_acl_type, the_acl);
				unbecome_root();
				if (sret == 0) {
					ret = True;	
				}
			}

			if (ret == False) {
				DEBUG(2,("set_canon_ace_list: sys_acl_set_file type %s failed for file %s (%s).\n",
						the_acl_type == SMB_ACL_TYPE_DEFAULT ? "directory default" : "file",
						fsp->fsp_name, strerror(errno) ));
				goto fail;
			}
		}
	} else {
		if (SMB_VFS_SYS_ACL_SET_FD(fsp, fsp->fh->fd, the_acl) == -1) {
			/*
			 * Some systems allow all the above calls and only fail with no ACL support
			 * when attempting to apply the acl. HPUX with HFS is an example of this. JRA.
			 */
			if (no_acl_syscall_error(errno)) {
				*pacl_set_support = False;
			}

			if (acl_group_override(conn, prim_gid)) {
				int sret;

				DEBUG(5,("set_canon_ace_list: acl group control on and current user in file %s primary group.\n",
					fsp->fsp_name ));

				become_root();
				sret = SMB_VFS_SYS_ACL_SET_FD(fsp, fsp->fh->fd, the_acl);
				unbecome_root();
				if (sret == 0) {
					ret = True;
				}
			}

			if (ret == False) {
				DEBUG(2,("set_canon_ace_list: sys_acl_set_file failed for file %s (%s).\n",
						fsp->fsp_name, strerror(errno) ));
				goto fail;
			}
		}
	}

	ret = True;

  fail:

	if (the_acl != NULL) {
		SMB_VFS_SYS_ACL_FREE_ACL(conn, the_acl);
	}

	return ret;
}

/****************************************************************************
 Find a particular canon_ace entry.
****************************************************************************/

static struct canon_ace *canon_ace_entry_for(struct canon_ace *list, SMB_ACL_TAG_T type, posix_id *id)
{
	while (list) {
		if (list->type == type && ((type != SMB_ACL_USER && type != SMB_ACL_GROUP) ||
				(type == SMB_ACL_USER  && id && id->uid == list->unix_ug.uid) ||
				(type == SMB_ACL_GROUP && id && id->gid == list->unix_ug.gid)))
			break;
		list = list->next;
	}
	return list;
}

/****************************************************************************
 
****************************************************************************/

SMB_ACL_T free_empty_sys_acl(connection_struct *conn, SMB_ACL_T the_acl)
{
	SMB_ACL_ENTRY_T entry;

	if (!the_acl)
		return NULL;
	if (SMB_VFS_SYS_ACL_GET_ENTRY(conn, the_acl, SMB_ACL_FIRST_ENTRY, &entry) != 1) {
		SMB_VFS_SYS_ACL_FREE_ACL(conn, the_acl);
		return NULL;
	}
	return the_acl;
}

/****************************************************************************
 Convert a canon_ace to a generic 3 element permission - if possible.
****************************************************************************/

#define MAP_PERM(p,mask,result) (((p) & (mask)) ? (result) : 0 )

static BOOL convert_canon_ace_to_posix_perms( files_struct *fsp, canon_ace *file_ace_list, mode_t *posix_perms)
{
	int snum = SNUM(fsp->conn);
	size_t ace_count = count_canon_ace_list(file_ace_list);
	canon_ace *ace_p;
	canon_ace *owner_ace = NULL;
	canon_ace *group_ace = NULL;
	canon_ace *other_ace = NULL;
	mode_t and_bits;
	mode_t or_bits;

	if (ace_count != 3) {
		DEBUG(3,("convert_canon_ace_to_posix_perms: Too many ACE entries for file %s to convert to \
posix perms.\n", fsp->fsp_name ));
		return False;
	}

	for (ace_p = file_ace_list; ace_p; ace_p = ace_p->next) {
		if (ace_p->owner_type == UID_ACE)
			owner_ace = ace_p;
		else if (ace_p->owner_type == GID_ACE)
			group_ace = ace_p;
		else if (ace_p->owner_type == WORLD_ACE)
			other_ace = ace_p;
	}

	if (!owner_ace || !group_ace || !other_ace) {
		DEBUG(3,("convert_canon_ace_to_posix_perms: Can't get standard entries for file %s.\n",
				fsp->fsp_name ));
		return False;
	}

	*posix_perms = (mode_t)0;

	*posix_perms |= owner_ace->perms;
	*posix_perms |= MAP_PERM(group_ace->perms, S_IRUSR, S_IRGRP);
	*posix_perms |= MAP_PERM(group_ace->perms, S_IWUSR, S_IWGRP);
	*posix_perms |= MAP_PERM(group_ace->perms, S_IXUSR, S_IXGRP);
	*posix_perms |= MAP_PERM(other_ace->perms, S_IRUSR, S_IROTH);
	*posix_perms |= MAP_PERM(other_ace->perms, S_IWUSR, S_IWOTH);
	*posix_perms |= MAP_PERM(other_ace->perms, S_IXUSR, S_IXOTH);

	/* The owner must have at least read access. */

	*posix_perms |= S_IRUSR;
	if (fsp->is_directory)
		*posix_perms |= (S_IWUSR|S_IXUSR);

	/* If requested apply the masks. */

	/* Get the initial bits to apply. */

	if (fsp->is_directory) {
		and_bits = lp_dir_security_mask(snum);
		or_bits = lp_force_dir_security_mode(snum);
	} else {
		and_bits = lp_security_mask(snum);
		or_bits = lp_force_security_mode(snum);
	}

	*posix_perms = (((*posix_perms) & and_bits)|or_bits);

	DEBUG(10,("convert_canon_ace_to_posix_perms: converted u=%o,g=%o,w=%o to perm=0%o for file %s.\n",
		(int)owner_ace->perms, (int)group_ace->perms, (int)other_ace->perms, (int)*posix_perms,
		fsp->fsp_name ));

	return True;
}

/****************************************************************************
  Incoming NT ACLs on a directory can be split into a default POSIX acl (CI|OI|IO) and
  a normal POSIX acl. Win2k needs these split acls re-merging into one ACL
  with CI|OI set so it is inherited and also applies to the directory.
  Based on code from "Jim McDonough" <jmcd@us.ibm.com>.
****************************************************************************/

static size_t merge_default_aces( SEC_ACE *nt_ace_list, size_t num_aces)
{
	size_t i, j;

	for (i = 0; i < num_aces; i++) {
		for (j = i+1; j < num_aces; j++) {
			uint32 i_flags_ni = (nt_ace_list[i].flags & ~SEC_ACE_FLAG_INHERITED_ACE);
			uint32 j_flags_ni = (nt_ace_list[j].flags & ~SEC_ACE_FLAG_INHERITED_ACE);
			BOOL i_inh = (nt_ace_list[i].flags & SEC_ACE_FLAG_INHERITED_ACE) ? True : False;
			BOOL j_inh = (nt_ace_list[j].flags & SEC_ACE_FLAG_INHERITED_ACE) ? True : False;

			/* We know the lower number ACE's are file entries. */
			if ((nt_ace_list[i].type == nt_ace_list[j].type) &&
				(nt_ace_list[i].size == nt_ace_list[j].size) &&
				(nt_ace_list[i].info.mask == nt_ace_list[j].info.mask) &&
				sid_equal(&nt_ace_list[i].trustee, &nt_ace_list[j].trustee) &&
				(i_inh == j_inh) &&
				(i_flags_ni == 0) &&
				(j_flags_ni == (SEC_ACE_FLAG_OBJECT_INHERIT|
						  SEC_ACE_FLAG_CONTAINER_INHERIT|
						  SEC_ACE_FLAG_INHERIT_ONLY))) {
				/*
				 * W2K wants to have access allowed zero access ACE's
				 * at the end of the list. If the mask is zero, merge
				 * the non-inherited ACE onto the inherited ACE.
				 */

				if (nt_ace_list[i].info.mask == 0) {
					nt_ace_list[j].flags = SEC_ACE_FLAG_OBJECT_INHERIT|SEC_ACE_FLAG_CONTAINER_INHERIT|
								(i_inh ? SEC_ACE_FLAG_INHERITED_ACE : 0);
					if (num_aces - i - 1 > 0)
						memmove(&nt_ace_list[i], &nt_ace_list[i+1], (num_aces-i-1) *
								sizeof(SEC_ACE));

					DEBUG(10,("merge_default_aces: Merging zero access ACE %u onto ACE %u.\n",
						(unsigned int)i, (unsigned int)j ));
				} else {
					/*
					 * These are identical except for the flags.
					 * Merge the inherited ACE onto the non-inherited ACE.
					 */

					nt_ace_list[i].flags = SEC_ACE_FLAG_OBJECT_INHERIT|SEC_ACE_FLAG_CONTAINER_INHERIT|
								(i_inh ? SEC_ACE_FLAG_INHERITED_ACE : 0);
					if (num_aces - j - 1 > 0)
						memmove(&nt_ace_list[j], &nt_ace_list[j+1], (num_aces-j-1) *
								sizeof(SEC_ACE));

					DEBUG(10,("merge_default_aces: Merging ACE %u onto ACE %u.\n",
						(unsigned int)j, (unsigned int)i ));
				}
				num_aces--;
				break;
			}
		}
	}

	return num_aces;
}
/****************************************************************************
 Reply to query a security descriptor from an fsp. If it succeeds it allocates
 the space for the return elements and returns the size needed to return the
 security descriptor. This should be the only external function needed for
 the UNIX style get ACL.
****************************************************************************/

size_t get_nt_acl(files_struct *fsp, uint32 security_info, SEC_DESC **ppdesc)
{
	connection_struct *conn = fsp->conn;
	SMB_STRUCT_STAT sbuf;
	SEC_ACE *nt_ace_list = NULL;
	DOM_SID owner_sid;
	DOM_SID group_sid;
	size_t sd_size = 0;
	SEC_ACL *psa = NULL;
	size_t num_acls = 0;
	size_t num_def_acls = 0;
	size_t num_aces = 0;
	SMB_ACL_T posix_acl = NULL;
	SMB_ACL_T def_acl = NULL;
	canon_ace *file_ace = NULL;
	canon_ace *dir_ace = NULL;
	size_t num_profile_acls = 0;
	struct pai_val *pal = NULL;
	SEC_DESC *psd = NULL;

	*ppdesc = NULL;

	DEBUG(10,("get_nt_acl: called for file %s\n", fsp->fsp_name ));

	if(fsp->is_directory || fsp->fh->fd == -1) {

		/* Get the stat struct for the owner info. */
		if(SMB_VFS_STAT(fsp->conn,fsp->fsp_name, &sbuf) != 0) {
			return 0;
		}
		/*
		 * Get the ACL from the path.
		 */

		posix_acl = SMB_VFS_SYS_ACL_GET_FILE(conn, fsp->fsp_name, SMB_ACL_TYPE_ACCESS);

		/*
		 * If it's a directory get the default POSIX ACL.
		 */

		if(fsp->is_directory) {
			def_acl = SMB_VFS_SYS_ACL_GET_FILE(conn, fsp->fsp_name, SMB_ACL_TYPE_DEFAULT);
			def_acl = free_empty_sys_acl(conn, def_acl);
		}

	} else {

		/* Get the stat struct for the owner info. */
		if(SMB_VFS_FSTAT(fsp,fsp->fh->fd,&sbuf) != 0) {
			return 0;
		}
		/*
		 * Get the ACL from the fd.
		 */
		posix_acl = SMB_VFS_SYS_ACL_GET_FD(fsp, fsp->fh->fd);
	}

	DEBUG(5,("get_nt_acl : file ACL %s, directory ACL %s\n",
			posix_acl ? "present" :  "absent",
			def_acl ? "present" :  "absent" ));

	pal = load_inherited_info(fsp);

	/*
	 * Get the owner, group and world SIDs.
	 */

	if (lp_profile_acls(SNUM(conn))) {
		/* For WXP SP1 the owner must be administrators. */
		sid_copy(&owner_sid, &global_sid_Builtin_Administrators);
		sid_copy(&group_sid, &global_sid_Builtin_Users);
		num_profile_acls = 2;
	} else {
		create_file_sids(&sbuf, &owner_sid, &group_sid);
	}

	if ((security_info & DACL_SECURITY_INFORMATION) && !(security_info & PROTECTED_DACL_SECURITY_INFORMATION)) {

		/*
		 * In the optimum case Creator Owner and Creator Group would be used for
		 * the ACL_USER_OBJ and ACL_GROUP_OBJ entries, respectively, but this
		 * would lead to usability problems under Windows: The Creator entries
		 * are only available in browse lists of directories and not for files;
		 * additionally the identity of the owning group couldn't be determined.
		 * We therefore use those identities only for Default ACLs. 
		 */

		/* Create the canon_ace lists. */
		file_ace = canonicalise_acl( fsp, posix_acl, &sbuf, &owner_sid, &group_sid, pal, SMB_ACL_TYPE_ACCESS );

		/* We must have *some* ACLS. */
	
		if (count_canon_ace_list(file_ace) == 0) {
			DEBUG(0,("get_nt_acl : No ACLs on file (%s) !\n", fsp->fsp_name ));
			goto done;
		}

		if (fsp->is_directory && def_acl) {
			dir_ace = canonicalise_acl(fsp, def_acl, &sbuf,
					&global_sid_Creator_Owner,
					&global_sid_Creator_Group, pal, SMB_ACL_TYPE_DEFAULT );
		}

		/*
		 * Create the NT ACE list from the canonical ace lists.
		 */

		{
			canon_ace *ace;
			int nt_acl_type;
			int i;

			if (nt4_compatible_acls() && dir_ace) {
				/*
				 * NT 4 chokes if an ACL contains an INHERIT_ONLY entry
				 * but no non-INHERIT_ONLY entry for one SID. So we only
				 * remove entries from the Access ACL if the
				 * corresponding Default ACL entries have also been
				 * removed. ACEs for CREATOR-OWNER and CREATOR-GROUP
				 * are exceptions. We can do nothing
				 * intelligent if the Default ACL contains entries that
				 * are not also contained in the Access ACL, so this
				 * case will still fail under NT 4.
				 */

				ace = canon_ace_entry_for(dir_ace, SMB_ACL_OTHER, NULL);
				if (ace && !ace->perms) {
					DLIST_REMOVE(dir_ace, ace);
					SAFE_FREE(ace);

					ace = canon_ace_entry_for(file_ace, SMB_ACL_OTHER, NULL);
					if (ace && !ace->perms) {
						DLIST_REMOVE(file_ace, ace);
						SAFE_FREE(ace);
					}
				}

				/*
				 * WinNT doesn't usually have Creator Group
				 * in browse lists, so we send this entry to
				 * WinNT even if it contains no relevant
				 * permissions. Once we can add
				 * Creator Group to browse lists we can
				 * re-enable this.
				 */

#if 0
				ace = canon_ace_entry_for(dir_ace, SMB_ACL_GROUP_OBJ, NULL);
				if (ace && !ace->perms) {
					DLIST_REMOVE(dir_ace, ace);
					SAFE_FREE(ace);
				}
#endif

				ace = canon_ace_entry_for(file_ace, SMB_ACL_GROUP_OBJ, NULL);
				if (ace && !ace->perms) {
					DLIST_REMOVE(file_ace, ace);
					SAFE_FREE(ace);
				}
			}

			num_acls = count_canon_ace_list(file_ace);
			num_def_acls = count_canon_ace_list(dir_ace);

			/* Allocate the ace list. */
			if ((nt_ace_list = SMB_MALLOC_ARRAY(SEC_ACE,num_acls + num_profile_acls + num_def_acls)) == NULL) {
				DEBUG(0,("get_nt_acl: Unable to malloc space for nt_ace_list.\n"));
				goto done;
			}

			memset(nt_ace_list, '\0', (num_acls + num_def_acls) * sizeof(SEC_ACE) );
											                
			/*
			 * Create the NT ACE list from the canonical ace lists.
			 */
	
			ace = file_ace;

			for (i = 0; i < num_acls; i++, ace = ace->next) {
				SEC_ACCESS acc;

				acc = map_canon_ace_perms(SNUM(conn), &nt_acl_type, &owner_sid, ace, fsp->is_directory);
				init_sec_ace(&nt_ace_list[num_aces++], &ace->trustee, nt_acl_type, acc, ace->inherited ? SEC_ACE_FLAG_INHERITED_ACE : 0);
			}

			/* The User must have access to a profile share - even if we can't map the SID. */
			if (lp_profile_acls(SNUM(conn))) {
				SEC_ACCESS acc;

				init_sec_access(&acc,FILE_GENERIC_ALL);
				init_sec_ace(&nt_ace_list[num_aces++], &global_sid_Builtin_Users, SEC_ACE_TYPE_ACCESS_ALLOWED,
						acc, 0);
			}

			ace = dir_ace;

			for (i = 0; i < num_def_acls; i++, ace = ace->next) {
				SEC_ACCESS acc;
	
				acc = map_canon_ace_perms(SNUM(conn), &nt_acl_type, &owner_sid, ace, fsp->is_directory);
				init_sec_ace(&nt_ace_list[num_aces++], &ace->trustee, nt_acl_type, acc,
						SEC_ACE_FLAG_OBJECT_INHERIT|SEC_ACE_FLAG_CONTAINER_INHERIT|
						SEC_ACE_FLAG_INHERIT_ONLY|
						(ace->inherited ? SEC_ACE_FLAG_INHERITED_ACE : 0));
			}

			/* The User must have access to a profile share - even if we can't map the SID. */
			if (lp_profile_acls(SNUM(conn))) {
				SEC_ACCESS acc;
			
				init_sec_access(&acc,FILE_GENERIC_ALL);
				init_sec_ace(&nt_ace_list[num_aces++], &global_sid_Builtin_Users, SEC_ACE_TYPE_ACCESS_ALLOWED, acc,
						SEC_ACE_FLAG_OBJECT_INHERIT|SEC_ACE_FLAG_CONTAINER_INHERIT|
						SEC_ACE_FLAG_INHERIT_ONLY|0);
			}

			/*
			 * Merge POSIX default ACLs and normal ACLs into one NT ACE.
			 * Win2K needs this to get the inheritance correct when replacing ACLs
			 * on a directory tree. Based on work by Jim @ IBM.
			 */

			num_aces = merge_default_aces(nt_ace_list, num_aces);

		}

		if (num_aces) {
			if((psa = make_sec_acl( main_loop_talloc_get(), NT4_ACL_REVISION, num_aces, nt_ace_list)) == NULL) {
				DEBUG(0,("get_nt_acl: Unable to malloc space for acl.\n"));
				goto done;
			}
		}
	} /* security_info & DACL_SECURITY_INFORMATION */

	psd = make_standard_sec_desc( main_loop_talloc_get(),
			(security_info & OWNER_SECURITY_INFORMATION) ? &owner_sid : NULL,
			(security_info & GROUP_SECURITY_INFORMATION) ? &group_sid : NULL,
			psa,
			&sd_size);

	if(!psd) {
		DEBUG(0,("get_nt_acl: Unable to malloc space for security descriptor.\n"));
		sd_size = 0;
		goto done;
	}

	/*
	 * Windows 2000: The DACL_PROTECTED flag in the security
	 * descriptor marks the ACL as non-inheriting, i.e., no
	 * ACEs from higher level directories propagate to this
	 * ACL. In the POSIX ACL model permissions are only
	 * inherited at file create time, so ACLs never contain
	 * any ACEs that are inherited dynamically. The DACL_PROTECTED
	 * flag doesn't seem to bother Windows NT.
	 * Always set this if map acl inherit is turned off.
	 */
	if (get_protected_flag(pal) || !lp_map_acl_inherit(SNUM(conn))) {
		psd->type |= SE_DESC_DACL_PROTECTED;
	}

	if (psd->dacl) {
		dacl_sort_into_canonical_order(psd->dacl->ace, (unsigned int)psd->dacl->num_aces);
	}

	*ppdesc = psd;

 done:

	if (posix_acl) {
		SMB_VFS_SYS_ACL_FREE_ACL(conn, posix_acl);
	}
	if (def_acl) {
		SMB_VFS_SYS_ACL_FREE_ACL(conn, def_acl);
	}
	free_canon_ace_list(file_ace);
	free_canon_ace_list(dir_ace);
	free_inherited_info(pal);
	SAFE_FREE(nt_ace_list);

	return sd_size;
}

/****************************************************************************
 Try to chown a file. We will be able to chown it under the following conditions.

  1) If we have root privileges, then it will just work.
  2) If we have SeTakeOwnershipPrivilege we can change the user to the current user.
  3) If we have SeRestorePrivilege we can change the user to any other user. 
  4) If we have write permission to the file and dos_filemodes is set
     then allow chown to the currently authenticated user.
****************************************************************************/

static int try_chown(connection_struct *conn, const char *fname, uid_t uid, gid_t gid)
{
	int ret;
	files_struct *fsp;
	SMB_STRUCT_STAT st;

	if(!CAN_WRITE(conn)) {
		return -1;
	}

	/* Case (1). */
	/* try the direct way first */
	ret = SMB_VFS_CHOWN(conn, fname, uid, gid);
	if (ret == 0)
		return 0;

	/* Case (2) / (3) */
	if (lp_enable_privileges()) {

		BOOL has_take_ownership_priv = user_has_privileges(current_user.nt_user_token,
							      &se_take_ownership);
		BOOL has_restore_priv = user_has_privileges(current_user.nt_user_token,
						       &se_restore);

		/* Case (2) */
		if ( ( has_take_ownership_priv && ( uid == current_user.ut.uid ) ) ||
		/* Case (3) */
		     ( has_restore_priv ) ) {

			become_root();
			/* Keep the current file gid the same - take ownership doesn't imply group change. */
			ret = SMB_VFS_CHOWN(conn, fname, uid, (gid_t)-1);
			unbecome_root();
			return ret;
		}
	}

	/* Case (4). */
	if (!lp_dos_filemode(SNUM(conn))) {
		return -1;
	}

	if (SMB_VFS_STAT(conn,fname,&st)) {
		return -1;
	}

	fsp = open_file_fchmod(conn,fname,&st);
	if (!fsp) {
		return -1;
	}

	/* only allow chown to the current user. This is more secure,
	   and also copes with the case where the SID in a take ownership ACL is
	   a local SID on the users workstation 
	*/
	uid = current_user.ut.uid;

	become_root();
	/* Keep the current file gid the same. */
	ret = SMB_VFS_FCHOWN(fsp, fsp->fh->fd, uid, (gid_t)-1);
	unbecome_root();

	close_file_fchmod(fsp);

	return ret;
}

/****************************************************************************
 Reply to set a security descriptor on an fsp. security_info_sent is the
 description of the following NT ACL.
 This should be the only external function needed for the UNIX style set ACL.
****************************************************************************/

BOOL set_nt_acl(files_struct *fsp, uint32 security_info_sent, SEC_DESC *psd)
{
	connection_struct *conn = fsp->conn;
	uid_t user = (uid_t)-1;
	gid_t grp = (gid_t)-1;
	SMB_STRUCT_STAT sbuf;  
	DOM_SID file_owner_sid;
	DOM_SID file_grp_sid;
	canon_ace *file_ace_list = NULL;
	canon_ace *dir_ace_list = NULL;
	BOOL acl_perms = False;
	mode_t orig_mode = (mode_t)0;
	uid_t orig_uid;
	gid_t orig_gid;
	BOOL need_chown = False;

	DEBUG(10,("set_nt_acl: called for file %s\n", fsp->fsp_name ));

	if (!CAN_WRITE(conn)) {
		DEBUG(10,("set acl rejected on read-only share\n"));
		return False;
	}

	/*
	 * Get the current state of the file.
	 */

	if(fsp->is_directory || fsp->fh->fd == -1) {
		if(SMB_VFS_STAT(fsp->conn,fsp->fsp_name, &sbuf) != 0)
			return False;
	} else {
		if(SMB_VFS_FSTAT(fsp,fsp->fh->fd,&sbuf) != 0)
			return False;
	}

	/* Save the original elements we check against. */
	orig_mode = sbuf.st_mode;
	orig_uid = sbuf.st_uid;
	orig_gid = sbuf.st_gid;

	/*
	 * Unpack the user/group/world id's.
	 */

	if (!unpack_nt_owners( SNUM(conn), &sbuf, &user, &grp, security_info_sent, psd)) {
		return False;
	}

	/*
	 * Do we need to chown ?
	 */

	if (((user != (uid_t)-1) && (orig_uid != user)) || (( grp != (gid_t)-1) && (orig_gid != grp))) {
		need_chown = True;
	}

	/*
	 * Chown before setting ACL only if we don't change the user, or
	 * if we change to the current user, but not if we want to give away
	 * the file.
	 */

	if (need_chown && (user == (uid_t)-1 || user == current_user.ut.uid)) {

		DEBUG(3,("set_nt_acl: chown %s. uid = %u, gid = %u.\n",
				fsp->fsp_name, (unsigned int)user, (unsigned int)grp ));

		if(try_chown( fsp->conn, fsp->fsp_name, user, grp) == -1) {
			DEBUG(3,("set_nt_acl: chown %s, %u, %u failed. Error = %s.\n",
				fsp->fsp_name, (unsigned int)user, (unsigned int)grp, strerror(errno) ));
			return False;
		}

		/*
		 * Recheck the current state of the file, which may have changed.
		 * (suid/sgid bits, for instance)
		 */

		if(fsp->is_directory) {
			if(SMB_VFS_STAT(fsp->conn, fsp->fsp_name, &sbuf) != 0) {
				return False;
			}
		} else {

			int ret;
    
			if(fsp->fh->fd == -1)
				ret = SMB_VFS_STAT(fsp->conn, fsp->fsp_name, &sbuf);
			else
				ret = SMB_VFS_FSTAT(fsp,fsp->fh->fd,&sbuf);
  
			if(ret != 0)
				return False;
		}

		/* Save the original elements we check against. */
		orig_mode = sbuf.st_mode;
		orig_uid = sbuf.st_uid;
		orig_gid = sbuf.st_gid;

		/* We did it, don't try again */
		need_chown = False;
	}

	create_file_sids(&sbuf, &file_owner_sid, &file_grp_sid);

	acl_perms = unpack_canon_ace( fsp, &sbuf, &file_owner_sid, &file_grp_sid,
					&file_ace_list, &dir_ace_list, security_info_sent, psd);

	/* Ignore W2K traverse DACL set. */
	if (file_ace_list || dir_ace_list) {

		if (!acl_perms) {
			DEBUG(3,("set_nt_acl: cannot set permissions\n"));
			free_canon_ace_list(file_ace_list);
			free_canon_ace_list(dir_ace_list); 
			return False;
		}

		/*
		 * Only change security if we got a DACL.
		 */

		if((security_info_sent & DACL_SECURITY_INFORMATION) && (psd->dacl != NULL)) {

			BOOL acl_set_support = False;
			BOOL ret = False;

			/*
			 * Try using the POSIX ACL set first. Fall back to chmod if
			 * we have no ACL support on this filesystem.
			 */

			if (acl_perms && file_ace_list) {
				ret = set_canon_ace_list(fsp, file_ace_list, False, sbuf.st_gid, &acl_set_support);
				if (acl_set_support && ret == False) {
					DEBUG(3,("set_nt_acl: failed to set file acl on file %s (%s).\n", fsp->fsp_name, strerror(errno) ));
					free_canon_ace_list(file_ace_list);
					free_canon_ace_list(dir_ace_list); 
					return False;
				}
			}

			if (acl_perms && acl_set_support && fsp->is_directory) {
				if (dir_ace_list) {
					if (!set_canon_ace_list(fsp, dir_ace_list, True, sbuf.st_gid, &acl_set_support)) {
						DEBUG(3,("set_nt_acl: failed to set default acl on directory %s (%s).\n", fsp->fsp_name, strerror(errno) ));
						free_canon_ace_list(file_ace_list);
						free_canon_ace_list(dir_ace_list); 
						return False;
					}
				} else {

					/*
					 * No default ACL - delete one if it exists.
					 */

					if (SMB_VFS_SYS_ACL_DELETE_DEF_FILE(conn, fsp->fsp_name) == -1) {
						int sret = -1;

						if (acl_group_override(conn, sbuf.st_gid)) {
							DEBUG(5,("set_nt_acl: acl group control on and "
								"current user in file %s primary group. Override delete_def_acl\n",
								fsp->fsp_name ));

							become_root();
							sret = SMB_VFS_SYS_ACL_DELETE_DEF_FILE(conn, fsp->fsp_name);
							unbecome_root();
						}

						if (sret == -1) {
							DEBUG(3,("set_nt_acl: sys_acl_delete_def_file failed (%s)\n", strerror(errno)));
							free_canon_ace_list(file_ace_list);
							free_canon_ace_list(dir_ace_list);
							return False;
						}
					}
				}
			}

			if (acl_set_support) {
				store_inheritance_attributes(fsp, file_ace_list, dir_ace_list,
						(psd->type & SE_DESC_DACL_PROTECTED) ? True : False);
			}

			/*
			 * If we cannot set using POSIX ACLs we fall back to checking if we need to chmod.
			 */

			if(!acl_set_support && acl_perms) {
				mode_t posix_perms;

				if (!convert_canon_ace_to_posix_perms( fsp, file_ace_list, &posix_perms)) {
					free_canon_ace_list(file_ace_list);
					free_canon_ace_list(dir_ace_list);
					DEBUG(3,("set_nt_acl: failed to convert file acl to posix permissions for file %s.\n",
						fsp->fsp_name ));
					return False;
				}

				if (orig_mode != posix_perms) {

					DEBUG(3,("set_nt_acl: chmod %s. perms = 0%o.\n",
						fsp->fsp_name, (unsigned int)posix_perms ));

					if(SMB_VFS_CHMOD(conn,fsp->fsp_name, posix_perms) == -1) {
						int sret = -1;
						if (acl_group_override(conn, sbuf.st_gid)) {
							DEBUG(5,("set_nt_acl: acl group control on and "
								"current user in file %s primary group. Override chmod\n",
								fsp->fsp_name ));

							become_root();
							sret = SMB_VFS_CHMOD(conn,fsp->fsp_name, posix_perms);
							unbecome_root();
						}

						if (sret == -1) {
							DEBUG(3,("set_nt_acl: chmod %s, 0%o failed. Error = %s.\n",
								fsp->fsp_name, (unsigned int)posix_perms, strerror(errno) ));
							free_canon_ace_list(file_ace_list);
							free_canon_ace_list(dir_ace_list);
							return False;
						}
					}
				}
			}
		}

		free_canon_ace_list(file_ace_list);
		free_canon_ace_list(dir_ace_list); 
	}

	/* Any chown pending? */
	if (need_chown) {

		DEBUG(3,("set_nt_acl: chown %s. uid = %u, gid = %u.\n",
			fsp->fsp_name, (unsigned int)user, (unsigned int)grp ));

		if(try_chown( fsp->conn, fsp->fsp_name, user, grp) == -1) {
			DEBUG(3,("set_nt_acl: chown %s, %u, %u failed. Error = %s.\n",
				fsp->fsp_name, (unsigned int)user, (unsigned int)grp, strerror(errno) ));
			return False;
		}
	}

	return True;
}

/****************************************************************************
 Get the actual group bits stored on a file with an ACL. Has no effect if
 the file has no ACL. Needed in dosmode code where the stat() will return
 the mask bits, not the real group bits, for a file with an ACL.
****************************************************************************/

int get_acl_group_bits( connection_struct *conn, const char *fname, mode_t *mode )
{
	int entry_id = SMB_ACL_FIRST_ENTRY;
	SMB_ACL_ENTRY_T entry;
	SMB_ACL_T posix_acl;
	int result = -1;

	posix_acl = SMB_VFS_SYS_ACL_GET_FILE(conn, fname, SMB_ACL_TYPE_ACCESS);
	if (posix_acl == (SMB_ACL_T)NULL)
		return -1;

	while (SMB_VFS_SYS_ACL_GET_ENTRY(conn, posix_acl, entry_id, &entry) == 1) {
		SMB_ACL_TAG_T tagtype;
		SMB_ACL_PERMSET_T permset;

		/* get_next... */
		if (entry_id == SMB_ACL_FIRST_ENTRY)
			entry_id = SMB_ACL_NEXT_ENTRY;

		if (SMB_VFS_SYS_ACL_GET_TAG_TYPE(conn, entry, &tagtype) ==-1)
			break;

		if (tagtype == SMB_ACL_GROUP_OBJ) {
			if (SMB_VFS_SYS_ACL_GET_PERMSET(conn, entry, &permset) == -1) {
				break;
			} else {
				*mode &= ~(S_IRGRP|S_IWGRP|S_IXGRP);
				*mode |= (SMB_VFS_SYS_ACL_GET_PERM(conn, permset, SMB_ACL_READ) ? S_IRGRP : 0);
				*mode |= (SMB_VFS_SYS_ACL_GET_PERM(conn, permset, SMB_ACL_WRITE) ? S_IWGRP : 0);
				*mode |= (SMB_VFS_SYS_ACL_GET_PERM(conn, permset, SMB_ACL_EXECUTE) ? S_IXGRP : 0);
				result = 0;
				break;
			}
		}
	}
	SMB_VFS_SYS_ACL_FREE_ACL(conn, posix_acl);
	return result;
}

/****************************************************************************
 Do a chmod by setting the ACL USER_OBJ, GROUP_OBJ and OTHER bits in an ACL
 and set the mask to rwx. Needed to preserve complex ACLs set by NT.
****************************************************************************/

static int chmod_acl_internals( connection_struct *conn, SMB_ACL_T posix_acl, mode_t mode)
{
	int entry_id = SMB_ACL_FIRST_ENTRY;
	SMB_ACL_ENTRY_T entry;
	int num_entries = 0;

	while ( SMB_VFS_SYS_ACL_GET_ENTRY(conn, posix_acl, entry_id, &entry) == 1) {
		SMB_ACL_TAG_T tagtype;
		SMB_ACL_PERMSET_T permset;
		mode_t perms;

		/* get_next... */
		if (entry_id == SMB_ACL_FIRST_ENTRY)
			entry_id = SMB_ACL_NEXT_ENTRY;

		if (SMB_VFS_SYS_ACL_GET_TAG_TYPE(conn, entry, &tagtype) == -1)
			return -1;

		if (SMB_VFS_SYS_ACL_GET_PERMSET(conn, entry, &permset) == -1)
			return -1;

		num_entries++;

		switch(tagtype) {
			case SMB_ACL_USER_OBJ:
				perms = unix_perms_to_acl_perms(mode, S_IRUSR, S_IWUSR, S_IXUSR);
				break;
			case SMB_ACL_GROUP_OBJ:
				perms = unix_perms_to_acl_perms(mode, S_IRGRP, S_IWGRP, S_IXGRP);
				break;
			case SMB_ACL_MASK:
				/*
				 * FIXME: The ACL_MASK entry permissions should really be set to
				 * the union of the permissions of all ACL_USER,
				 * ACL_GROUP_OBJ, and ACL_GROUP entries. That's what
				 * acl_calc_mask() does, but Samba ACLs doesn't provide it.
				 */
				perms = S_IRUSR|S_IWUSR|S_IXUSR;
				break;
			case SMB_ACL_OTHER:
				perms = unix_perms_to_acl_perms(mode, S_IROTH, S_IWOTH, S_IXOTH);
				break;
			default:
				continue;
		}

		if (map_acl_perms_to_permset(conn, perms, &permset) == -1)
			return -1;

		if (SMB_VFS_SYS_ACL_SET_PERMSET(conn, entry, permset) == -1)
			return -1;
	}

	/*
	 * If this is a simple 3 element ACL or no elements then it's a standard
	 * UNIX permission set. Just use chmod...	
	 */

	if ((num_entries == 3) || (num_entries == 0))
		return -1;

	return 0;
}

/****************************************************************************
 Get the access ACL of FROM, do a chmod by setting the ACL USER_OBJ,
 GROUP_OBJ and OTHER bits in an ACL and set the mask to rwx. Set the
 resulting ACL on TO.  Note that name is in UNIX character set.
****************************************************************************/

static int copy_access_acl(connection_struct *conn, const char *from, const char *to, mode_t mode)
{
	SMB_ACL_T posix_acl = NULL;
	int ret = -1;

	if ((posix_acl = SMB_VFS_SYS_ACL_GET_FILE(conn, from, SMB_ACL_TYPE_ACCESS)) == NULL)
		return -1;

	if ((ret = chmod_acl_internals(conn, posix_acl, mode)) == -1)
		goto done;

	ret = SMB_VFS_SYS_ACL_SET_FILE(conn, to, SMB_ACL_TYPE_ACCESS, posix_acl);

 done:

	SMB_VFS_SYS_ACL_FREE_ACL(conn, posix_acl);
	return ret;
}

/****************************************************************************
 Do a chmod by setting the ACL USER_OBJ, GROUP_OBJ and OTHER bits in an ACL
 and set the mask to rwx. Needed to preserve complex ACLs set by NT.
 Note that name is in UNIX character set.
****************************************************************************/

int chmod_acl(connection_struct *conn, const char *name, mode_t mode)
{
	return copy_access_acl(conn, name, name, mode);
}

/****************************************************************************
 If "inherit permissions" is set and the parent directory has no default
 ACL but it does have an Access ACL, inherit this Access ACL to file name.
****************************************************************************/

int inherit_access_acl(connection_struct *conn, const char *name, mode_t mode)
{
	pstring dirname;
	pstrcpy(dirname, parent_dirname(name));

	if (!lp_inherit_perms(SNUM(conn)) || directory_has_default_acl(conn, dirname))
		return 0;

	return copy_access_acl(conn, dirname, name, mode);
}

/****************************************************************************
 Do an fchmod by setting the ACL USER_OBJ, GROUP_OBJ and OTHER bits in an ACL
 and set the mask to rwx. Needed to preserve complex ACLs set by NT.
****************************************************************************/

int fchmod_acl(files_struct *fsp, int fd, mode_t mode)
{
	connection_struct *conn = fsp->conn;
	SMB_ACL_T posix_acl = NULL;
	int ret = -1;

	if ((posix_acl = SMB_VFS_SYS_ACL_GET_FD(fsp, fd)) == NULL)
		return -1;

	if ((ret = chmod_acl_internals(conn, posix_acl, mode)) == -1)
		goto done;

	ret = SMB_VFS_SYS_ACL_SET_FD(fsp, fd, posix_acl);

  done:

	SMB_VFS_SYS_ACL_FREE_ACL(conn, posix_acl);
	return ret;
}

/****************************************************************************
 Check for an existing default POSIX ACL on a directory.
****************************************************************************/

BOOL directory_has_default_acl(connection_struct *conn, const char *fname)
{
	SMB_ACL_T def_acl = SMB_VFS_SYS_ACL_GET_FILE( conn, fname, SMB_ACL_TYPE_DEFAULT);
	BOOL has_acl = False;
	SMB_ACL_ENTRY_T entry;

	if (def_acl != NULL && (SMB_VFS_SYS_ACL_GET_ENTRY(conn, def_acl, SMB_ACL_FIRST_ENTRY, &entry) == 1)) {
		has_acl = True;
	}

	if (def_acl) {
	        SMB_VFS_SYS_ACL_FREE_ACL(conn, def_acl);
	}
        return has_acl;
}

/****************************************************************************
 Map from wire type to permset.
****************************************************************************/

static BOOL unix_ex_wire_to_permset(connection_struct *conn, unsigned char wire_perm, SMB_ACL_PERMSET_T *p_permset)
{
	if (wire_perm & ~(SMB_POSIX_ACL_READ|SMB_POSIX_ACL_WRITE|SMB_POSIX_ACL_EXECUTE)) {
		return False;
	}

	if (SMB_VFS_SYS_ACL_CLEAR_PERMS(conn, *p_permset) ==  -1) {
		return False;
	}

	if (wire_perm & SMB_POSIX_ACL_READ) {
		if (SMB_VFS_SYS_ACL_ADD_PERM(conn, *p_permset, SMB_ACL_READ) == -1) {
			return False;
		}
	}
	if (wire_perm & SMB_POSIX_ACL_WRITE) {
		if (SMB_VFS_SYS_ACL_ADD_PERM(conn, *p_permset, SMB_ACL_WRITE) == -1) {
			return False;
		}
	}
	if (wire_perm & SMB_POSIX_ACL_EXECUTE) {
		if (SMB_VFS_SYS_ACL_ADD_PERM(conn, *p_permset, SMB_ACL_EXECUTE) == -1) {
			return False;
		}
	}
	return True;
}

/****************************************************************************
 Map from wire type to tagtype.
****************************************************************************/

static BOOL unix_ex_wire_to_tagtype(unsigned char wire_tt, SMB_ACL_TAG_T *p_tt)
{
	switch (wire_tt) {
		case SMB_POSIX_ACL_USER_OBJ:
			*p_tt = SMB_ACL_USER_OBJ;
			break;
		case SMB_POSIX_ACL_USER:
			*p_tt = SMB_ACL_USER;
			break;
		case SMB_POSIX_ACL_GROUP_OBJ:
			*p_tt = SMB_ACL_GROUP_OBJ;
			break;
		case SMB_POSIX_ACL_GROUP:
			*p_tt = SMB_ACL_GROUP;
			break;
		case SMB_POSIX_ACL_MASK:
			*p_tt = SMB_ACL_MASK;
			break;
		case SMB_POSIX_ACL_OTHER:
			*p_tt = SMB_ACL_OTHER;
			break;
		default:
			return False;
	}
	return True;
}

/****************************************************************************
 Create a new POSIX acl from wire permissions.
 FIXME ! How does the share mask/mode fit into this.... ?
****************************************************************************/

static SMB_ACL_T create_posix_acl_from_wire(connection_struct *conn, uint16 num_acls, const char *pdata)
{
	unsigned int i;
	SMB_ACL_T the_acl = SMB_VFS_SYS_ACL_INIT(conn, num_acls);

	if (the_acl == NULL) {
		return NULL;
	}

	for (i = 0; i < num_acls; i++) {
		SMB_ACL_ENTRY_T the_entry;
		SMB_ACL_PERMSET_T the_permset;
		SMB_ACL_TAG_T tag_type;

		if (SMB_VFS_SYS_ACL_CREATE_ENTRY(conn, &the_acl, &the_entry) == -1) {
			DEBUG(0,("create_posix_acl_from_wire: Failed to create entry %u. (%s)\n",
				i, strerror(errno) ));
			goto fail;
		}

		if (!unix_ex_wire_to_tagtype(CVAL(pdata,(i*SMB_POSIX_ACL_ENTRY_SIZE)), &tag_type)) {
			DEBUG(0,("create_posix_acl_from_wire: invalid wire tagtype %u on entry %u.\n",
				CVAL(pdata,(i*SMB_POSIX_ACL_ENTRY_SIZE)), i ));
			goto fail;
		}

		if (SMB_VFS_SYS_ACL_SET_TAG_TYPE(conn, the_entry, tag_type) == -1) {
			DEBUG(0,("create_posix_acl_from_wire: Failed to set tagtype on entry %u. (%s)\n",
				i, strerror(errno) ));
			goto fail;
		}

		/* Get the permset pointer from the new ACL entry. */
		if (SMB_VFS_SYS_ACL_GET_PERMSET(conn, the_entry, &the_permset) == -1) {
			DEBUG(0,("create_posix_acl_from_wire: Failed to get permset on entry %u. (%s)\n",
                                i, strerror(errno) ));
                        goto fail;
                }

		/* Map from wire to permissions. */
		if (!unix_ex_wire_to_permset(conn, CVAL(pdata,(i*SMB_POSIX_ACL_ENTRY_SIZE)+1), &the_permset)) {
			DEBUG(0,("create_posix_acl_from_wire: invalid permset %u on entry %u.\n",
				CVAL(pdata,(i*SMB_POSIX_ACL_ENTRY_SIZE) + 1), i ));
			goto fail;
		}

		/* Now apply to the new ACL entry. */
		if (SMB_VFS_SYS_ACL_SET_PERMSET(conn, the_entry, the_permset) == -1) {
			DEBUG(0,("create_posix_acl_from_wire: Failed to add permset on entry %u. (%s)\n",
				i, strerror(errno) ));
			goto fail;
		}

		if (tag_type == SMB_ACL_USER) {
			uint32 uidval = IVAL(pdata,(i*SMB_POSIX_ACL_ENTRY_SIZE)+2);
			uid_t uid = (uid_t)uidval;
			if (SMB_VFS_SYS_ACL_SET_QUALIFIER(conn, the_entry,(void *)&uid) == -1) {
				DEBUG(0,("create_posix_acl_from_wire: Failed to set uid %u on entry %u. (%s)\n",
					(unsigned int)uid, i, strerror(errno) ));
				goto fail;
			}
		}

		if (tag_type == SMB_ACL_GROUP) {
			uint32 gidval = IVAL(pdata,(i*SMB_POSIX_ACL_ENTRY_SIZE)+2);
			gid_t gid = (uid_t)gidval;
			if (SMB_VFS_SYS_ACL_SET_QUALIFIER(conn, the_entry,(void *)&gid) == -1) {
				DEBUG(0,("create_posix_acl_from_wire: Failed to set gid %u on entry %u. (%s)\n",
					(unsigned int)gid, i, strerror(errno) ));
				goto fail;
			}
		}
	}

	return the_acl;

 fail:

	if (the_acl != NULL) {
		SMB_VFS_SYS_ACL_FREE_ACL(conn, the_acl);
	}
	return NULL;
}

/****************************************************************************
 Calls from UNIX extensions - Default POSIX ACL set.
 If num_def_acls == 0 and not a directory just return. If it is a directory
 and num_def_acls == 0 then remove the default acl. Else set the default acl
 on the directory.
****************************************************************************/

BOOL set_unix_posix_default_acl(connection_struct *conn, const char *fname, SMB_STRUCT_STAT *psbuf,
				uint16 num_def_acls, const char *pdata)
{
	SMB_ACL_T def_acl = NULL;

	if (num_def_acls && !S_ISDIR(psbuf->st_mode)) {
		DEBUG(5,("set_unix_posix_default_acl: Can't set default ACL on non-directory file %s\n", fname ));
		errno = EISDIR;
		return False;
	}

	if (!num_def_acls) {
		/* Remove the default ACL. */
		if (SMB_VFS_SYS_ACL_DELETE_DEF_FILE(conn, fname) == -1) {
			DEBUG(5,("set_unix_posix_default_acl: acl_delete_def_file failed on directory %s (%s)\n",
				fname, strerror(errno) ));
			return False;
		}
		return True;
	}

	if ((def_acl = create_posix_acl_from_wire(conn, num_def_acls, pdata)) == NULL) {
		return False;
	}

	if (SMB_VFS_SYS_ACL_SET_FILE(conn, fname, SMB_ACL_TYPE_DEFAULT, def_acl) == -1) {
		DEBUG(5,("set_unix_posix_default_acl: acl_set_file failed on directory %s (%s)\n",
			fname, strerror(errno) ));
	        SMB_VFS_SYS_ACL_FREE_ACL(conn, def_acl);
		return False;
	}

	DEBUG(10,("set_unix_posix_default_acl: set default acl for file %s\n", fname ));
	SMB_VFS_SYS_ACL_FREE_ACL(conn, def_acl);
	return True;
}

/****************************************************************************
 Remove an ACL from a file. As we don't have acl_delete_entry() available
 we must read the current acl and copy all entries except MASK, USER and GROUP
 to a new acl, then set that. This (at least on Linux) causes any ACL to be
 removed.
 FIXME ! How does the share mask/mode fit into this.... ?
****************************************************************************/

static BOOL remove_posix_acl(connection_struct *conn, files_struct *fsp, const char *fname)
{
	SMB_ACL_T file_acl = NULL;
	int entry_id = SMB_ACL_FIRST_ENTRY;
	SMB_ACL_ENTRY_T entry;
	BOOL ret = False;
	/* Create a new ACL with only 3 entries, u/g/w. */
	SMB_ACL_T new_file_acl = SMB_VFS_SYS_ACL_INIT(conn, 3);
	SMB_ACL_ENTRY_T user_ent = NULL;
	SMB_ACL_ENTRY_T group_ent = NULL;
	SMB_ACL_ENTRY_T other_ent = NULL;

	if (new_file_acl == NULL) {
		DEBUG(5,("remove_posix_acl: failed to init new ACL with 3 entries for file %s.\n", fname));
		return False;
	}

	/* Now create the u/g/w entries. */
	if (SMB_VFS_SYS_ACL_CREATE_ENTRY(conn, &new_file_acl, &user_ent) == -1) {
		DEBUG(5,("remove_posix_acl: Failed to create user entry for file %s. (%s)\n",
			fname, strerror(errno) ));
		goto done;
	}
	if (SMB_VFS_SYS_ACL_SET_TAG_TYPE(conn, user_ent, SMB_ACL_USER_OBJ) == -1) {
		DEBUG(5,("remove_posix_acl: Failed to set user entry for file %s. (%s)\n",
			fname, strerror(errno) ));
		goto done;
	}

	if (SMB_VFS_SYS_ACL_CREATE_ENTRY(conn, &new_file_acl, &group_ent) == -1) {
		DEBUG(5,("remove_posix_acl: Failed to create group entry for file %s. (%s)\n",
			fname, strerror(errno) ));
		goto done;
	}
	if (SMB_VFS_SYS_ACL_SET_TAG_TYPE(conn, group_ent, SMB_ACL_GROUP_OBJ) == -1) {
		DEBUG(5,("remove_posix_acl: Failed to set group entry for file %s. (%s)\n",
			fname, strerror(errno) ));
		goto done;
	}

	if (SMB_VFS_SYS_ACL_CREATE_ENTRY(conn, &new_file_acl, &other_ent) == -1) {
		DEBUG(5,("remove_posix_acl: Failed to create other entry for file %s. (%s)\n",
			fname, strerror(errno) ));
		goto done;
	}
	if (SMB_VFS_SYS_ACL_SET_TAG_TYPE(conn, other_ent, SMB_ACL_OTHER) == -1) {
		DEBUG(5,("remove_posix_acl: Failed to set other entry for file %s. (%s)\n",
			fname, strerror(errno) ));
		goto done;
	}

	/* Get the current file ACL. */
	if (fsp && fsp->fh->fd != -1) {
		file_acl = SMB_VFS_SYS_ACL_GET_FD(fsp, fsp->fh->fd);
	} else {
		file_acl = SMB_VFS_SYS_ACL_GET_FILE( conn, fname, SMB_ACL_TYPE_ACCESS);
	}

	if (file_acl == NULL) {
		/* This is only returned if an error occurred. Even for a file with
		   no acl a u/g/w acl should be returned. */
		DEBUG(5,("remove_posix_acl: failed to get ACL from file %s (%s).\n",
			fname, strerror(errno) ));
		goto done;
	}

	while ( SMB_VFS_SYS_ACL_GET_ENTRY(conn, file_acl, entry_id, &entry) == 1) {
		SMB_ACL_TAG_T tagtype;
		SMB_ACL_PERMSET_T permset;

		/* get_next... */
		if (entry_id == SMB_ACL_FIRST_ENTRY)
			entry_id = SMB_ACL_NEXT_ENTRY;

		if (SMB_VFS_SYS_ACL_GET_TAG_TYPE(conn, entry, &tagtype) == -1) {
			DEBUG(5,("remove_posix_acl: failed to get tagtype from ACL on file %s (%s).\n",
				fname, strerror(errno) ));
			goto done;
		}

		if (SMB_VFS_SYS_ACL_GET_PERMSET(conn, entry, &permset) == -1) {
			DEBUG(5,("remove_posix_acl: failed to get permset from ACL on file %s (%s).\n",
				fname, strerror(errno) ));
			goto done;
		}

		if (tagtype == SMB_ACL_USER_OBJ) {
			if (SMB_VFS_SYS_ACL_SET_PERMSET(conn, user_ent, permset) == -1) {
				DEBUG(5,("remove_posix_acl: failed to set permset from ACL on file %s (%s).\n",
					fname, strerror(errno) ));
			}
		} else if (tagtype == SMB_ACL_GROUP_OBJ) {
			if (SMB_VFS_SYS_ACL_SET_PERMSET(conn, group_ent, permset) == -1) {
				DEBUG(5,("remove_posix_acl: failed to set permset from ACL on file %s (%s).\n",
					fname, strerror(errno) ));
			}
		} else if (tagtype == SMB_ACL_OTHER) {
			if (SMB_VFS_SYS_ACL_SET_PERMSET(conn, other_ent, permset) == -1) {
				DEBUG(5,("remove_posix_acl: failed to set permset from ACL on file %s (%s).\n",
					fname, strerror(errno) ));
			}
		}
	}

	/* Set the new empty file ACL. */
	if (fsp && fsp->fh->fd != -1) {
		if (SMB_VFS_SYS_ACL_SET_FD(fsp, fsp->fh->fd, new_file_acl) == -1) {
			DEBUG(5,("remove_posix_acl: acl_set_file failed on %s (%s)\n",
				fname, strerror(errno) ));
			goto done;
		}
	} else {
		if (SMB_VFS_SYS_ACL_SET_FILE(conn, fname, SMB_ACL_TYPE_ACCESS, new_file_acl) == -1) {
			DEBUG(5,("remove_posix_acl: acl_set_file failed on %s (%s)\n",
				fname, strerror(errno) ));
			goto done;
		}
	}

	ret = True;

 done:

	if (file_acl) {
		SMB_VFS_SYS_ACL_FREE_ACL(conn, file_acl);
	}
	if (new_file_acl) {
		SMB_VFS_SYS_ACL_FREE_ACL(conn, new_file_acl);
	}
	return ret;
}

/****************************************************************************
 Calls from UNIX extensions - POSIX ACL set.
 If num_def_acls == 0 then read/modify/write acl after removing all entries
 except SMB_ACL_USER_OBJ, SMB_ACL_GROUP_OBJ, SMB_ACL_OTHER.
****************************************************************************/

BOOL set_unix_posix_acl(connection_struct *conn, files_struct *fsp, const char *fname, uint16 num_acls, const char *pdata)
{
	SMB_ACL_T file_acl = NULL;

	if (!num_acls) {
		/* Remove the ACL from the file. */
		return remove_posix_acl(conn, fsp, fname);
	}

	if ((file_acl = create_posix_acl_from_wire(conn, num_acls, pdata)) == NULL) {
		return False;
	}

	if (fsp && fsp->fh->fd != -1) {
		/* The preferred way - use an open fd. */
		if (SMB_VFS_SYS_ACL_SET_FD(fsp, fsp->fh->fd, file_acl) == -1) {
			DEBUG(5,("set_unix_posix_acl: acl_set_file failed on %s (%s)\n",
				fname, strerror(errno) ));
		        SMB_VFS_SYS_ACL_FREE_ACL(conn, file_acl);
			return False;
		}
	} else {
		if (SMB_VFS_SYS_ACL_SET_FILE(conn, fname, SMB_ACL_TYPE_ACCESS, file_acl) == -1) {
			DEBUG(5,("set_unix_posix_acl: acl_set_file failed on %s (%s)\n",
				fname, strerror(errno) ));
		        SMB_VFS_SYS_ACL_FREE_ACL(conn, file_acl);
			return False;
		}
	}

	DEBUG(10,("set_unix_posix_acl: set acl for file %s\n", fname ));
	SMB_VFS_SYS_ACL_FREE_ACL(conn, file_acl);
	return True;
}

/****************************************************************************
 Check for POSIX group ACLs. If none use stat entry.
 Return -1 if no match, 0 if match and denied, 1 if match and allowed.
****************************************************************************/

static int check_posix_acl_group_access(connection_struct *conn, const char *fname, SMB_STRUCT_STAT *psbuf, uint32 access_mask)
{
	SMB_ACL_T posix_acl = NULL;
	int entry_id = SMB_ACL_FIRST_ENTRY;
	SMB_ACL_ENTRY_T entry;
	int i;
	BOOL seen_mask = False;
	BOOL seen_owning_group = False;
	int ret = -1;
	gid_t cu_gid;

	DEBUG(10,("check_posix_acl_group_access: requesting 0x%x on file %s\n",
		(unsigned int)access_mask, fname ));

	if ((posix_acl = SMB_VFS_SYS_ACL_GET_FILE(conn, fname, SMB_ACL_TYPE_ACCESS)) == NULL) {
		goto check_stat;
	}

	/* First ensure the group mask allows group access. */
	/* Also check any user entries (these take preference over group). */

	while ( SMB_VFS_SYS_ACL_GET_ENTRY(conn, posix_acl, entry_id, &entry) == 1) {
		SMB_ACL_TAG_T tagtype;
		SMB_ACL_PERMSET_T permset;
		int have_write = -1;
		int have_read = -1;

		/* get_next... */
		if (entry_id == SMB_ACL_FIRST_ENTRY)
			entry_id = SMB_ACL_NEXT_ENTRY;

		if (SMB_VFS_SYS_ACL_GET_TAG_TYPE(conn, entry, &tagtype) == -1) {
			goto check_stat;
		}

		if (SMB_VFS_SYS_ACL_GET_PERMSET(conn, entry, &permset) == -1) {
			goto check_stat;
		}

		have_read = SMB_VFS_SYS_ACL_GET_PERM(conn, permset, SMB_ACL_READ);
		if (have_read == -1) {
			goto check_stat;
		}

		have_write = SMB_VFS_SYS_ACL_GET_PERM(conn, permset, SMB_ACL_WRITE);
		if (have_write == -1) {
			goto check_stat;
		}

		/*
		 * Solaris returns 2 for this if write is available.
		 * canonicalize to 0 or 1.
		 */	
		have_write = (have_write ? 1 : 0);
		have_read = (have_read ? 1 : 0);

		switch(tagtype) {
			case SMB_ACL_MASK:
				seen_mask = True;
				switch (access_mask) {
					case FILE_READ_DATA:
						if (!have_read) {
							ret = -1;
							DEBUG(10,("check_posix_acl_group_access: file %s "
								"refusing read due to mask.\n", fname));
							goto done;
						}
						break;
					case FILE_WRITE_DATA:
						if (!have_write) {
							ret = -1;
							DEBUG(10,("check_posix_acl_group_access: file %s "
								"refusing write due to mask.\n", fname));
							goto done;
						}
						break;
					default: /* FILE_READ_DATA|FILE_WRITE_DATA */
						if (!have_write || !have_read) {
							ret = -1;
							DEBUG(10,("check_posix_acl_group_access: file %s "
								"refusing read/write due to mask.\n", fname));
							goto done;
						}
						break;
				}
				break;
			case SMB_ACL_USER:
			{
				/* Check against current_user.ut.uid. */
				uid_t *puid = (uid_t *)SMB_VFS_SYS_ACL_GET_QUALIFIER(conn, entry);
				if (puid == NULL) {
					goto check_stat;
				}
				if (current_user.ut.uid == *puid) {
					/* We have a uid match but we must ensure we have seen the acl mask. */
					switch (access_mask) {
						case FILE_READ_DATA:
							ret = have_read;
							break;
						case FILE_WRITE_DATA:
							ret = have_write;
							break;
						default: /* FILE_READ_DATA|FILE_WRITE_DATA */
							ret = (have_write & have_read);
							break;
					}
					DEBUG(10,("check_posix_acl_group_access: file %s "
						"match on user %u -> %s.\n",
						fname, (unsigned int)*puid,
						ret ? "can access" : "cannot access"));
					if (seen_mask) {
						goto done;
					}
				}
				break;
			}
			default:
				continue;
		}
	}

	/* If ret is anything other than -1 we matched on a user entry. */
	if (ret != -1) {
		goto done;
	}

	/* Next check all group entries. */
	entry_id = SMB_ACL_FIRST_ENTRY;
	while ( SMB_VFS_SYS_ACL_GET_ENTRY(conn, posix_acl, entry_id, &entry) == 1) {
		SMB_ACL_TAG_T tagtype;
		SMB_ACL_PERMSET_T permset;
		int have_write = -1;
		int have_read = -1;

		/* get_next... */
		if (entry_id == SMB_ACL_FIRST_ENTRY)
			entry_id = SMB_ACL_NEXT_ENTRY;

		if (SMB_VFS_SYS_ACL_GET_TAG_TYPE(conn, entry, &tagtype) == -1) {
			goto check_stat;
		}

		if (SMB_VFS_SYS_ACL_GET_PERMSET(conn, entry, &permset) == -1) {
			goto check_stat;
		}

		have_read = SMB_VFS_SYS_ACL_GET_PERM(conn, permset, SMB_ACL_READ);
		if (have_read == -1) {
			goto check_stat;
		}

		have_write = SMB_VFS_SYS_ACL_GET_PERM(conn, permset, SMB_ACL_WRITE);
		if (have_write == -1) {
			goto check_stat;
		}

		/*
		 * Solaris returns 2 for this if write is available.
		 * canonicalize to 0 or 1.
		 */	
		have_write = (have_write ? 1 : 0);
		have_read = (have_read ? 1 : 0);

		switch(tagtype) {
			case SMB_ACL_GROUP:
			case SMB_ACL_GROUP_OBJ:
			{
				gid_t *pgid = NULL;

				if (tagtype == SMB_ACL_GROUP) {
					pgid = (gid_t *)SMB_VFS_SYS_ACL_GET_QUALIFIER(conn, entry);
				} else {
					seen_owning_group = True;
					pgid = &psbuf->st_gid;
				}
				if (pgid == NULL) {
					goto check_stat;
				}

				/*
				 * Does it match the current effective group
				 * or supplementary groups ?
				 */
				for (cu_gid = get_current_user_gid_first(&i); cu_gid != (gid_t)-1;
							cu_gid = get_current_user_gid_next(&i)) {
					if (cu_gid == *pgid) {
						switch (access_mask) {
							case FILE_READ_DATA:
								ret = have_read;
								break;
							case FILE_WRITE_DATA:
								ret = have_write;
								break;
							default: /* FILE_READ_DATA|FILE_WRITE_DATA */
								ret = (have_write & have_read);
								break;
						}

						DEBUG(10,("check_posix_acl_group_access: file %s "
							"match on group %u -> can access.\n",
							fname, (unsigned int)cu_gid ));

						/* If we don't have access permission this entry doesn't
							terminate the enumeration of the entries. */
						if (ret) {
							goto done;
						}
						/* But does terminate the group iteration. */
						break;
					}
				}
				break;
			}
			default:
				continue;
		}
	}

	/* If ret is -1 here we didn't match on the user entry or
	   supplemental group entries. */
	
	DEBUG(10,("check_posix_acl_group_access: ret = %d before check_stat:\n", ret));

  check_stat:

	/*
	 * We only check the S_I[RW]GRP permissions if we haven't already
	 * seen an owning group SMB_ACL_GROUP_OBJ ace entry. If there is an
	 * SMB_ACL_GROUP_OBJ ace entry then the group bits in st_gid are
	 * the same as the SMB_ACL_MASK bits, not the SMB_ACL_GROUP_OBJ
	 * bits. Thanks to Marc Cousin <mcousin@sigma.fr> for pointing
	 * this out. JRA.
	 */

	if (!seen_owning_group) {
		/* Do we match on the owning group entry ? */
		/*
		 * Does it match the current effective group
		 * or supplementary groups ?
		 */
		for (cu_gid = get_current_user_gid_first(&i); cu_gid != (gid_t)-1;
						cu_gid = get_current_user_gid_next(&i)) {
			if (cu_gid == psbuf->st_gid) {
				switch (access_mask) {
					case FILE_READ_DATA:
						ret = (psbuf->st_mode & S_IRGRP) ? 1 : 0;
						break;
					case FILE_WRITE_DATA:
						ret = (psbuf->st_mode & S_IWGRP) ? 1 : 0;
						break;
					default: /* FILE_READ_DATA|FILE_WRITE_DATA */
						if ((psbuf->st_mode & (S_IWGRP|S_IRGRP)) == (S_IWGRP|S_IRGRP)) {
							ret = 1;
						} else {
							ret = 0;
						}
						break;
				}
				DEBUG(10,("check_posix_acl_group_access: file %s "
					"match on owning group %u -> %s.\n",
					fname, (unsigned int)psbuf->st_gid,
					ret ? "can access" : "cannot access"));
				break;
			}
		}

		if (cu_gid == (gid_t)-1) {
			DEBUG(10,("check_posix_acl_group_access: file %s "
				"failed to match on user or group in token (ret = %d).\n",
				fname, ret ));
		}
	}

  done:

	if (posix_acl) {
		SMB_VFS_SYS_ACL_FREE_ACL(conn, posix_acl);
	}

	DEBUG(10,("check_posix_acl_group_access: file %s returning (ret = %d).\n", fname, ret ));
	return ret;
}

/****************************************************************************
 Actually emulate the in-kernel access checking for delete access. We need
 this to successfully return ACCESS_DENIED on a file open for delete access.
****************************************************************************/

BOOL can_delete_file_in_directory(connection_struct *conn, const char *fname)
{
	SMB_STRUCT_STAT sbuf;  
	pstring dname;
	int ret;

	if (!CAN_WRITE(conn)) {
		return False;
	}

	/* Get the parent directory permission mask and owners. */
	pstrcpy(dname, parent_dirname(fname));
	if(SMB_VFS_STAT(conn, dname, &sbuf) != 0) {
		return False;
	}
	if (!S_ISDIR(sbuf.st_mode)) {
		return False;
	}
	if (current_user.ut.uid == 0 || conn->admin_user) {
		/* I'm sorry sir, I didn't know you were root... */
		return True;
	}

	/* Check primary owner write access. */
	if (current_user.ut.uid == sbuf.st_uid) {
		return (sbuf.st_mode & S_IWUSR) ? True : False;
	}

#ifdef S_ISVTX
	/* sticky bit means delete only by owner or root. */
	if (sbuf.st_mode & S_ISVTX) {
		SMB_STRUCT_STAT sbuf_file;  
		if(SMB_VFS_STAT(conn, fname, &sbuf_file) != 0) {
			return False;
		}
		/*
		 * Patch from SATOH Fumiyasu <fumiyas@miraclelinux.com>
		 * for bug #3348. Don't assume owning sticky bit
		 * directory means write access allowed.
		 */
		if (current_user.ut.uid != sbuf_file.st_uid) {
			return False;
		}
	}
#endif

	/* Check group or explicit user acl entry write access. */
	ret = check_posix_acl_group_access(conn, dname, &sbuf, FILE_WRITE_DATA);
	if (ret == 0 || ret == 1) {
		return ret ? True : False;
	}

	/* Finally check other write access. */
	return (sbuf.st_mode & S_IWOTH) ? True : False;
}

/****************************************************************************
 Actually emulate the in-kernel access checking for read/write access. We need
 this to successfully check for ability to write for dos filetimes.
 Note this doesn't take into account share write permissions.
****************************************************************************/

BOOL can_access_file(connection_struct *conn, const char *fname, SMB_STRUCT_STAT *psbuf, uint32 access_mask)
{
	int ret;

	if (!(access_mask & (FILE_READ_DATA|FILE_WRITE_DATA))) {
		return False;
	}
	access_mask &= (FILE_READ_DATA|FILE_WRITE_DATA);

	DEBUG(10,("can_access_file: requesting 0x%x on file %s\n",
		(unsigned int)access_mask, fname ));

	if (current_user.ut.uid == 0 || conn->admin_user) {
		/* I'm sorry sir, I didn't know you were root... */
		return True;
	}

	if (!VALID_STAT(*psbuf)) {
		/* Get the file permission mask and owners. */
		if(SMB_VFS_STAT(conn, fname, psbuf) != 0) {
			return False;
		}
	}

	/* Check primary owner access. */
	if (current_user.ut.uid == psbuf->st_uid) {
		switch (access_mask) {
			case FILE_READ_DATA:
				return (psbuf->st_mode & S_IRUSR) ? True : False;

			case FILE_WRITE_DATA:
				return (psbuf->st_mode & S_IWUSR) ? True : False;

			default: /* FILE_READ_DATA|FILE_WRITE_DATA */

				if ((psbuf->st_mode & (S_IWUSR|S_IRUSR)) == (S_IWUSR|S_IRUSR)) {
					return True;
				} else {
					return False;
				}
		}
	}

	/* Check group or explicit user acl entry access. */
	ret = check_posix_acl_group_access(conn, fname, psbuf, access_mask);
	if (ret == 0 || ret == 1) {
		return ret ? True : False;
	}

	/* Finally check other access. */
	switch (access_mask) {
		case FILE_READ_DATA:
			return (psbuf->st_mode & S_IROTH) ? True : False;

		case FILE_WRITE_DATA:
			return (psbuf->st_mode & S_IWOTH) ? True : False;

		default: /* FILE_READ_DATA|FILE_WRITE_DATA */

			if ((psbuf->st_mode & (S_IWOTH|S_IROTH)) == (S_IWOTH|S_IROTH)) {
				return True;
			}
	}
	return False;
}

/****************************************************************************
 Userspace check for write access.
 Note this doesn't take into account share write permissions.
****************************************************************************/

BOOL can_write_to_file(connection_struct *conn, const char *fname, SMB_STRUCT_STAT *psbuf)
{
	return can_access_file(conn, fname, psbuf, FILE_WRITE_DATA);
}

/********************************************************************
 Pull the NT ACL from a file on disk or the OpenEventlog() access
 check.  Caller is responsible for freeing the returned security
 descriptor via TALLOC_FREE().  This is designed for dealing with 
 user space access checks in smbd outside of the VFS.  For example,
 checking access rights in OpenEventlog().
 
 Assume we are dealing with files (for now)
********************************************************************/

SEC_DESC* get_nt_acl_no_snum( TALLOC_CTX *ctx, const char *fname)
{
	SEC_DESC *psd, *ret_sd;
	connection_struct conn;
	files_struct finfo;
	struct fd_handle fh;
	pstring path;
	pstring filename;
	
	ZERO_STRUCT( conn );
	conn.service = -1;
	
	if ( !(conn.mem_ctx = talloc_init( "novfs_get_nt_acl" )) ) {
		DEBUG(0,("get_nt_acl_no_snum: talloc() failed!\n"));
		return NULL;
	}
	
	pstrcpy( path, "/" );
	set_conn_connectpath(&conn, path);
	
	if (!smbd_vfs_init(&conn)) {
		DEBUG(0,("get_nt_acl_no_snum: Unable to create a fake connection struct!\n"));
		conn_free_internal( &conn );
		return NULL;
        }
	
	ZERO_STRUCT( finfo );
	ZERO_STRUCT( fh );
	
	finfo.fnum = -1;
	finfo.conn = &conn;
	finfo.fh = &fh;
	finfo.fh->fd = -1;
	pstrcpy( filename, fname );
	finfo.fsp_name = filename;
	
	if (get_nt_acl( &finfo, DACL_SECURITY_INFORMATION, &psd ) == 0) {
		DEBUG(0,("get_nt_acl_no_snum: get_nt_acl returned zero.\n"));
		conn_free_internal( &conn );
		return NULL;
	}
	
	ret_sd = dup_sec_desc( ctx, psd );
	
	conn_free_internal( &conn );
	
	return ret_sd;
}
