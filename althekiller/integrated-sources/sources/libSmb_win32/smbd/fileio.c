/* 
   Unix SMB/Netbios implementation.
   Version 1.9.
   read/write to a files_struct
   Copyright (C) Andrew Tridgell 1992-1998
   Copyright (C) Jeremy Allison 2000-2002. - write cache.
   
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

static BOOL setup_write_cache(files_struct *, SMB_OFF_T);

/****************************************************************************
 Read from write cache if we can.
****************************************************************************/

static BOOL read_from_write_cache(files_struct *fsp,char *data,SMB_OFF_T pos,size_t n)
{
	write_cache *wcp = fsp->wcp;

	if(!wcp) {
		return False;
	}

	if( n > wcp->data_size || pos < wcp->offset || pos + n > wcp->offset + wcp->data_size) {
		return False;
	}

	memcpy(data, wcp->data + (pos - wcp->offset), n);

	DO_PROFILE_INC(writecache_read_hits);

	return True;
}

/****************************************************************************
 Read from a file.
****************************************************************************/

ssize_t read_file(files_struct *fsp,char *data,SMB_OFF_T pos,size_t n)
{
	ssize_t ret=0,readret;

	/* you can't read from print files */
	if (fsp->print_file) {
		return -1;
	}

	/*
	 * Serve from write cache if we can.
	 */

	if(read_from_write_cache(fsp, data, pos, n)) {
		fsp->fh->pos = pos + n;
		fsp->fh->position_information = fsp->fh->pos;
		return n;
	}

	flush_write_cache(fsp, READ_FLUSH);

	fsp->fh->pos = pos;

	if (n > 0) {
#ifdef DMF_FIX
		int numretries = 3;
tryagain:
		readret = SMB_VFS_PREAD(fsp,fsp->fh->fd,data,n,pos);

		if (readret == -1) {
			if ((errno == EAGAIN) && numretries) {
				DEBUG(3,("read_file EAGAIN retry in 10 seconds\n"));
				(void)sleep(10);
				--numretries;
				goto tryagain;
			}
			return -1;
		}
#else /* NO DMF fix. */
		readret = SMB_VFS_PREAD(fsp,fsp->fh->fd,data,n,pos);

		if (readret == -1) {
			return -1;
		}
#endif
		if (readret > 0) {
			ret += readret;
		}
	}

	DEBUG(10,("read_file (%s): pos = %.0f, size = %lu, returned %lu\n",
		fsp->fsp_name, (double)pos, (unsigned long)n, (long)ret ));

	fsp->fh->pos += ret;
	fsp->fh->position_information = fsp->fh->pos;

	return(ret);
}

/* how many write cache buffers have been allocated */
static unsigned int allocated_write_caches;

/****************************************************************************
 *Really* write to a file.
****************************************************************************/

static ssize_t real_write_file(files_struct *fsp,const char *data, SMB_OFF_T pos, size_t n)
{
	ssize_t ret;

        if (pos == -1) {
                ret = vfs_write_data(fsp, data, n);
        } else {
		fsp->fh->pos = pos;
		if (pos && lp_strict_allocate(SNUM(fsp->conn))) {
			if (vfs_fill_sparse(fsp, pos) == -1) {
				return -1;
			}
		}
                ret = vfs_pwrite_data(fsp, data, n, pos);
	}

	DEBUG(10,("real_write_file (%s): pos = %.0f, size = %lu, returned %ld\n",
		fsp->fsp_name, (double)pos, (unsigned long)n, (long)ret ));

	if (ret != -1) {
		fsp->fh->pos += ret;

		/*
		 * It turns out that setting the last write time from a Windows
		 * client stops any subsequent writes from updating the write time.
		 * Doing this after the write gives a race condition here where
		 * a stat may see the changed write time before we reset it here,
		 * but it's cheaper than having to store the write time in shared
		 * memory and look it up using dev/inode across all running smbd's.
		 * The 99% solution will hopefully be good enough in this case. JRA.
		 */

		if (fsp->pending_modtime) {
			set_filetime(fsp->conn, fsp->fsp_name, fsp->pending_modtime);

			/* If we didn't get the "set modtime" call ourselves, we must
			   store the last write time to restore on close. JRA. */
			if (!fsp->pending_modtime_owner) {
				fsp->last_write_time = time(NULL);
			}
		}

/* Yes - this is correct - writes don't update this. JRA. */
/* Found by Samba4 tests. */
#if 0
		fsp->position_information = fsp->pos;
#endif
	}

	return ret;
}

/****************************************************************************
 File size cache change.
 Updates size on disk but doesn't flush the cache.
****************************************************************************/

static int wcp_file_size_change(files_struct *fsp)
{
	int ret;
	write_cache *wcp = fsp->wcp;

	wcp->file_size = wcp->offset + wcp->data_size;
	ret = SMB_VFS_FTRUNCATE(fsp, fsp->fh->fd, wcp->file_size);
	if (ret == -1) {
		DEBUG(0,("wcp_file_size_change (%s): ftruncate of size %.0f error %s\n",
			fsp->fsp_name, (double)wcp->file_size, strerror(errno) ));
	}
	return ret;
}

/****************************************************************************
 Write to a file.
****************************************************************************/

ssize_t write_file(files_struct *fsp, const char *data, SMB_OFF_T pos, size_t n)
{
	write_cache *wcp = fsp->wcp;
	ssize_t total_written = 0;
	int write_path = -1; 

	if (fsp->print_file) {
		fstring sharename;
		uint32 jobid;

		if (!rap_to_pjobid(fsp->rap_print_jobid, sharename, &jobid)) {
			DEBUG(3,("write_file: Unable to map RAP jobid %u to jobid.\n",
						(unsigned int)fsp->rap_print_jobid ));
			errno = EBADF;
			return -1;
		}

		return print_job_write(SNUM(fsp->conn), jobid, data, pos, n);
	}

	if (!fsp->can_write) {
		errno = EPERM;
		return(0);
	}

	if (!fsp->modified) {
		SMB_STRUCT_STAT st;
		fsp->modified = True;

		if (SMB_VFS_FSTAT(fsp,fsp->fh->fd,&st) == 0) {
			int dosmode = dos_mode(fsp->conn,fsp->fsp_name,&st);
			if ((lp_store_dos_attributes(SNUM(fsp->conn)) || MAP_ARCHIVE(fsp->conn)) && !IS_DOS_ARCHIVE(dosmode)) {
				file_set_dosmode(fsp->conn,fsp->fsp_name,dosmode | aARCH,&st, False);
			}

			/*
			 * If this is the first write and we have an exclusive oplock then setup
			 * the write cache.
			 */

			if (EXCLUSIVE_OPLOCK_TYPE(fsp->oplock_type) && !wcp) {
				setup_write_cache(fsp, st.st_size);
				wcp = fsp->wcp;
			} 
		}  
	}

#ifdef WITH_PROFILE
	DO_PROFILE_INC(writecache_total_writes);
	if (!fsp->oplock_type) {
		DO_PROFILE_INC(writecache_non_oplock_writes);
	}
#endif

	/*
	 * If this file is level II oplocked then we need
	 * to grab the shared memory lock and inform all
	 * other files with a level II lock that they need
	 * to flush their read caches. We keep the lock over
	 * the shared memory area whilst doing this.
	 */

	release_level_2_oplocks_on_change(fsp);

#ifdef WITH_PROFILE
	if (profile_p && profile_p->writecache_total_writes % 500 == 0) {
		DEBUG(3,("WRITECACHE: initwrites=%u abutted=%u total=%u \
nonop=%u allocated=%u active=%u direct=%u perfect=%u readhits=%u\n",
			profile_p->writecache_init_writes,
			profile_p->writecache_abutted_writes,
			profile_p->writecache_total_writes,
			profile_p->writecache_non_oplock_writes,
			profile_p->writecache_allocated_write_caches,
			profile_p->writecache_num_write_caches,
			profile_p->writecache_direct_writes,
			profile_p->writecache_num_perfect_writes,
			profile_p->writecache_read_hits ));

		DEBUG(3,("WRITECACHE: Flushes SEEK=%d, READ=%d, WRITE=%d, READRAW=%d, OPLOCK=%d, CLOSE=%d, SYNC=%d\n",
			profile_p->writecache_flushed_writes[SEEK_FLUSH],
			profile_p->writecache_flushed_writes[READ_FLUSH],
			profile_p->writecache_flushed_writes[WRITE_FLUSH],
			profile_p->writecache_flushed_writes[READRAW_FLUSH],
			profile_p->writecache_flushed_writes[OPLOCK_RELEASE_FLUSH],
			profile_p->writecache_flushed_writes[CLOSE_FLUSH],
			profile_p->writecache_flushed_writes[SYNC_FLUSH] ));
	}
#endif

	if(!wcp) {
		DO_PROFILE_INC(writecache_direct_writes);
		total_written = real_write_file(fsp, data, pos, n);
		return total_written;
	}

	DEBUG(9,("write_file (%s)(fd=%d pos=%.0f size=%u) wcp->offset=%.0f wcp->data_size=%u\n",
		fsp->fsp_name, fsp->fh->fd, (double)pos, (unsigned int)n, (double)wcp->offset, (unsigned int)wcp->data_size));

	fsp->fh->pos = pos + n;

	/* 
	 * If we have active cache and it isn't contiguous then we flush.
	 * NOTE: There is a small problem with running out of disk ....
	 */

	if (wcp->data_size) {
		BOOL cache_flush_needed = False;

		if ((pos >= wcp->offset) && (pos <= wcp->offset + wcp->data_size)) {
      
			/* ASCII art.... JRA.

      +--------------+-----
      | Cached data  | Rest of allocated cache buffer....
      +--------------+-----

            +-------------------+
            | Data to write     |
            +-------------------+

	    		*/

			/*
			 * Start of write overlaps or abutts the existing data.
			 */

			size_t data_used = MIN((wcp->alloc_size - (pos - wcp->offset)), n);

			memcpy(wcp->data + (pos - wcp->offset), data, data_used);

			/*
			 * Update the current buffer size with the new data.
			 */

			if(pos + data_used > wcp->offset + wcp->data_size) {
				wcp->data_size = pos + data_used - wcp->offset;
			}

			/*
			 * Update the file size if changed.
			 */

			if (wcp->offset + wcp->data_size > wcp->file_size) {
				if (wcp_file_size_change(fsp) == -1) {
					return -1;
				}
			}

			/*
			 * If we used all the data then
			 * return here.
			 */

			if(n == data_used) {
				return n;
			} else {
				cache_flush_needed = True;
			}
			/*
			 * Move the start of data forward by the amount used,
			 * cut down the amount left by the same amount.
			 */

			data += data_used;
			pos += data_used;
			n -= data_used;

			DO_PROFILE_INC(writecache_abutted_writes);
			total_written = data_used;

			write_path = 1;

		} else if ((pos < wcp->offset) && (pos + n > wcp->offset) && 
					(pos + n <= wcp->offset + wcp->alloc_size)) {

			/* ASCII art.... JRA.

                        +---------------+
                        | Cache buffer  |
                        +---------------+

            +-------------------+
            | Data to write     |
            +-------------------+

	    		*/

			/*
			 * End of write overlaps the existing data.
			 */

			size_t data_used = pos + n - wcp->offset;

			memcpy(wcp->data, data + n - data_used, data_used);

			/*
			 * Update the current buffer size with the new data.
			 */

			if(pos + n > wcp->offset + wcp->data_size) {
				wcp->data_size = pos + n - wcp->offset;
			}

			/*
			 * Update the file size if changed.
			 */

			if (wcp->offset + wcp->data_size > wcp->file_size) {
				if (wcp_file_size_change(fsp) == -1) {
					return -1;
				}
			}

			/*
			 * We don't need to move the start of data, but we
			 * cut down the amount left by the amount used.
			 */

			n -= data_used;

			/*
			 * We cannot have used all the data here.
			 */

			cache_flush_needed = True;

			DO_PROFILE_INC(writecache_abutted_writes);
			total_written = data_used;

			write_path = 2;

		} else if ( (pos >= wcp->file_size) && 
					(wcp->offset + wcp->data_size == wcp->file_size) &&
					(pos > wcp->offset + wcp->data_size) && 
					(pos < wcp->offset + wcp->alloc_size) ) {

			/* ASCII art.... JRA.

                       End of file ---->|

                        +---------------+---------------+
                        | Cached data   | Cache buffer  |
                        +---------------+---------------+

                                              +-------------------+
                                              | Data to write     |
                                              +-------------------+

	    		*/

			/*
			 * Non-contiguous write part of which fits within
			 * the cache buffer and is extending the file
			 * and the cache contents reflect the current
			 * data up to the current end of the file.
			 */

			size_t data_used;

			if(pos + n <= wcp->offset + wcp->alloc_size) {
				data_used = n;
			} else {
				data_used = wcp->offset + wcp->alloc_size - pos;
			}

			/*
			 * Fill in the non-continuous area with zeros.
			 */

			memset(wcp->data + wcp->data_size, '\0',
				pos - (wcp->offset + wcp->data_size) );

			memcpy(wcp->data + (pos - wcp->offset), data, data_used);

			/*
			 * Update the current buffer size with the new data.
			 */

			if(pos + data_used > wcp->offset + wcp->data_size) {
				wcp->data_size = pos + data_used - wcp->offset;
			}

			/*
			 * Update the file size if changed.
			 */

			if (wcp->offset + wcp->data_size > wcp->file_size) {
				if (wcp_file_size_change(fsp) == -1) {
					return -1;
				}
			}

			/*
			 * If we used all the data then
			 * return here.
			 */

			if(n == data_used) {
				return n;
			} else {
				cache_flush_needed = True;
			}

			/*
			 * Move the start of data forward by the amount used,
			 * cut down the amount left by the same amount.
			 */

			data += data_used;
			pos += data_used;
			n -= data_used;

			DO_PROFILE_INC(writecache_abutted_writes);
			total_written = data_used;

			write_path = 3;

                } else if ( (pos >= wcp->file_size) && 
			    (n == 1) &&
			    (pos < wcp->offset + 2*wcp->alloc_size) &&
			    (wcp->file_size == wcp->offset + wcp->data_size)) {

                        /*
                        +---------------+
                        | Cached data   |
                        +---------------+

                                                         +--------+
                                                         | 1 Byte |
                                                         +--------+

			MS-Office seems to do this a lot to determine if there's enough
			space on the filesystem to write a new file.
                        */

			SMB_BIG_UINT new_start = wcp->offset + wcp->data_size;

			flush_write_cache(fsp, WRITE_FLUSH);
			wcp->offset = new_start;
			wcp->data_size = pos - new_start + 1;
			memset(wcp->data, '\0', wcp->data_size);
			memcpy(wcp->data + wcp->data_size-1, data, 1);

			/*
			 * Update the file size if changed.
			 */

			if (wcp->offset + wcp->data_size > wcp->file_size) {
				if (wcp_file_size_change(fsp) == -1) {
					return -1;
				}
			}

			return n;

		} else {

			/* ASCII art..... JRA.

   Case 1).

                        +---------------+---------------+
                        | Cached data   | Cache buffer  |
                        +---------------+---------------+

                                                              +-------------------+
                                                              | Data to write     |
                                                              +-------------------+

   Case 2).

                           +---------------+---------------+
                           | Cached data   | Cache buffer  |
                           +---------------+---------------+

   +-------------------+
   | Data to write     |
   +-------------------+

    Case 3).

                           +---------------+---------------+
                           | Cached data   | Cache buffer  |
                           +---------------+---------------+

                  +-----------------------------------------------------+
                  | Data to write                                       |
                  +-----------------------------------------------------+

		  */

 			/*
			 * Write is bigger than buffer, or there is no overlap on the
			 * low or high ends.
			 */

			DEBUG(9,("write_file: non cacheable write : fd = %d, pos = %.0f, len = %u, current cache pos = %.0f \
len = %u\n",fsp->fh->fd, (double)pos, (unsigned int)n, (double)wcp->offset, (unsigned int)wcp->data_size ));

			/*
			 * If write would fit in the cache, and is larger than
			 * the data already in the cache, flush the cache and
			 * preferentially copy the data new data into it. Otherwise
			 * just write the data directly.
			 */

			if ( n <= wcp->alloc_size && n > wcp->data_size) {
				cache_flush_needed = True;
			} else {
				ssize_t ret = real_write_file(fsp, data, pos, n);

				/*
				 * If the write overlaps the entire cache, then
				 * discard the current contents of the cache.
				 * Fix from Rasmus Borup Hansen rbh@math.ku.dk.
				 */

				if ((pos <= wcp->offset) &&
						(pos + n >= wcp->offset + wcp->data_size) ) {
					DEBUG(9,("write_file: discarding overwritten write \
cache: fd = %d, off=%.0f, size=%u\n", fsp->fh->fd, (double)wcp->offset, (unsigned int)wcp->data_size ));
					wcp->data_size = 0;
				}

				DO_PROFILE_INC(writecache_direct_writes);
				if (ret == -1) {
					return ret;
				}

				if (pos + ret > wcp->file_size) {
					wcp->file_size = pos + ret;
				}

				return ret;
			}

			write_path = 4;

		}

		if (cache_flush_needed) {
			DEBUG(3,("WRITE_FLUSH:%d: due to noncontinuous write: fd = %d, size = %.0f, pos = %.0f, \
n = %u, wcp->offset=%.0f, wcp->data_size=%u\n",
				write_path, fsp->fh->fd, (double)wcp->file_size, (double)pos, (unsigned int)n,
				(double)wcp->offset, (unsigned int)wcp->data_size ));

			flush_write_cache(fsp, WRITE_FLUSH);
		}
	}

	/*
	 * If the write request is bigger than the cache
	 * size, write it all out.
	 */

	if (n > wcp->alloc_size ) {
		ssize_t ret = real_write_file(fsp, data, pos, n);
		if (ret == -1) {
			return -1;
		}

		if (pos + ret > wcp->file_size) {
			wcp->file_size = pos + n;
		}

		DO_PROFILE_INC(writecache_direct_writes);
		return total_written + n;
	}

	/*
	 * If there's any data left, cache it.
	 */

	if (n) {
#ifdef WITH_PROFILE
		if (wcp->data_size) {
			DO_PROFILE_INC(writecache_abutted_writes);
		} else {
			DO_PROFILE_INC(writecache_init_writes);
		}
#endif
		memcpy(wcp->data+wcp->data_size, data, n);
		if (wcp->data_size == 0) {
			wcp->offset = pos;
			DO_PROFILE_INC(writecache_num_write_caches);
		}
		wcp->data_size += n;

		/*
		 * Update the file size if changed.
		 */

		if (wcp->offset + wcp->data_size > wcp->file_size) {
			if (wcp_file_size_change(fsp) == -1) {
				return -1;
			}
		}
		DEBUG(9,("wcp->offset = %.0f wcp->data_size = %u cache return %u\n",
			(double)wcp->offset, (unsigned int)wcp->data_size, (unsigned int)n));

		total_written += n;
		return total_written; /* .... that's a write :) */
	}
  
	return total_written;
}

/****************************************************************************
 Delete the write cache structure.
****************************************************************************/

void delete_write_cache(files_struct *fsp)
{
	write_cache *wcp;

	if(!fsp) {
		return;
	}

	if(!(wcp = fsp->wcp)) {
		return;
	}

	DO_PROFILE_DEC(writecache_allocated_write_caches);
	allocated_write_caches--;

	SMB_ASSERT(wcp->data_size == 0);

	SAFE_FREE(wcp->data);
	SAFE_FREE(fsp->wcp);

	DEBUG(10,("delete_write_cache: File %s deleted write cache\n", fsp->fsp_name ));
}

/****************************************************************************
 Setup the write cache structure.
****************************************************************************/

static BOOL setup_write_cache(files_struct *fsp, SMB_OFF_T file_size)
{
	ssize_t alloc_size = lp_write_cache_size(SNUM(fsp->conn));
	write_cache *wcp;

	if (allocated_write_caches >= MAX_WRITE_CACHES) {
		return False;
	}

	if(alloc_size == 0 || fsp->wcp) {
		return False;
	}

	if((wcp = SMB_MALLOC_P(write_cache)) == NULL) {
		DEBUG(0,("setup_write_cache: malloc fail.\n"));
		return False;
	}

	wcp->file_size = file_size;
	wcp->offset = 0;
	wcp->alloc_size = alloc_size;
	wcp->data_size = 0;
	if((wcp->data = (char *)SMB_MALLOC(wcp->alloc_size)) == NULL) {
		DEBUG(0,("setup_write_cache: malloc fail for buffer size %u.\n",
			(unsigned int)wcp->alloc_size ));
		SAFE_FREE(wcp);
		return False;
	}

	memset(wcp->data, '\0', wcp->alloc_size );

	fsp->wcp = wcp;
	DO_PROFILE_INC(writecache_allocated_write_caches);
	allocated_write_caches++;

	DEBUG(10,("setup_write_cache: File %s allocated write cache size %lu\n",
		fsp->fsp_name, (unsigned long)wcp->alloc_size ));

	return True;
}

/****************************************************************************
 Cope with a size change.
****************************************************************************/

void set_filelen_write_cache(files_struct *fsp, SMB_OFF_T file_size)
{
	if(fsp->wcp) {
		/* The cache *must* have been flushed before we do this. */
		if (fsp->wcp->data_size != 0) {
			pstring msg;
			slprintf(msg, sizeof(msg)-1, "set_filelen_write_cache: size change \
on file %s with write cache size = %lu\n", fsp->fsp_name, (unsigned long)fsp->wcp->data_size );
			smb_panic(msg);
		}
		fsp->wcp->file_size = file_size;
	}
}

/*******************************************************************
 Flush a write cache struct to disk.
********************************************************************/

ssize_t flush_write_cache(files_struct *fsp, enum flush_reason_enum reason)
{
	write_cache *wcp = fsp->wcp;
	size_t data_size;
	ssize_t ret;

	if(!wcp || !wcp->data_size) {
		return 0;
	}

	data_size = wcp->data_size;
	wcp->data_size = 0;

	DO_PROFILE_DEC_INC(writecache_num_write_caches,writecache_flushed_writes[reason]);

	DEBUG(9,("flushing write cache: fd = %d, off=%.0f, size=%u\n",
		fsp->fh->fd, (double)wcp->offset, (unsigned int)data_size));

#ifdef WITH_PROFILE
	if(data_size == wcp->alloc_size) {
		DO_PROFILE_INC(writecache_num_perfect_writes);
	}
#endif

	ret = real_write_file(fsp, wcp->data, wcp->offset, data_size);

	/*
	 * Ensure file size if kept up to date if write extends file.
	 */

	if ((ret != -1) && (wcp->offset + ret > wcp->file_size)) {
		wcp->file_size = wcp->offset + ret;
	}

	return ret;
}

/*******************************************************************
sync a file
********************************************************************/

void sync_file(connection_struct *conn, files_struct *fsp, BOOL write_through)
{
       	if (fsp->fh->fd == -1)
		return;

	if (lp_strict_sync(SNUM(conn)) &&
	    (lp_syncalways(SNUM(conn)) || write_through)) {
		flush_write_cache(fsp, SYNC_FLUSH);
		SMB_VFS_FSYNC(fsp,fsp->fh->fd);
	}
}

/************************************************************
 Perform a stat whether a valid fd or not.
************************************************************/

int fsp_stat(files_struct *fsp, SMB_STRUCT_STAT *pst)
{
	if (fsp->fh->fd == -1) {
		return SMB_VFS_STAT(fsp->conn, fsp->fsp_name, pst);
	} else {
		return SMB_VFS_FSTAT(fsp,fsp->fh->fd, pst);
	}
}
