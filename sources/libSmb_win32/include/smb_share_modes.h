/*
   Samba share mode database library.

   Copyright (C) Jeremy Allison 2005.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _SMB_SHARE_MODES_H_
#define _SMB_STATE_MODES_H_

#ifdef __cplusplus
extern "C" {
#endif

#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif

#include "tdb.h"

/* Database context handle. */
struct smbdb_ctx {
	TDB_CONTEXT *smb_tdb;
};

/* Share mode entry. */
/*
 We use 64 bit types for device and inode as
 we don't know what size mode Samba has been
 compiled in - dev/ino may be 32, may be 64
 bits. This interface copes with either.
*/
  
struct smb_share_mode_entry {
	uint64_t dev;
	uint64_t ino;
	uint32_t share_access;
	uint32_t access_mask;
	struct timeval open_time;
	uint32_t file_id;
	struct process_id pid;
};

/*
 * open/close sharemode database.
 */

struct smbdb_ctx *smb_share_mode_db_open(const char *db_path);
int smb_share_mode_db_close(struct smbdb_ctx *db_ctx);

/*
 * lock/unlock entry in sharemode database.
 */

int smb_lock_share_mode_entry(struct smbdb_ctx *db_ctx,
				uint64_t dev,
				uint64_t ino);

int smb_unlock_share_mode_entry(struct smbdb_ctx *db_ctx,
				uint64_t dev,
				uint64_t ino);

/*
 * Share mode database accessor functions.
 */

int smb_get_share_mode_entries(struct smbdb_ctx *db_ctx,
				uint64_t dev,
				uint64_t ino,
				struct smb_share_mode_entry **pp_list,
				unsigned char *p_delete_on_close);

int smb_create_share_mode_entry(struct smbdb_ctx *db_ctx,
				uint64_t dev,
				uint64_t ino,
				const struct smb_share_mode_entry *set_entry,
				const char *path);

int smb_delete_share_mode_entry(struct smbdb_ctx *db_ctx,
				uint64_t dev,
				uint64_t ino,
				const struct smb_share_mode_entry *set_entry);

int smb_change_share_mode_entry(struct smbdb_ctx *db_ctx,
				uint64_t dev,
				uint64_t ino,
				const struct smb_share_mode_entry *set_entry,
				const struct smb_share_mode_entry *new_entry);

#ifdef __cplusplus
}
#endif
#endif
