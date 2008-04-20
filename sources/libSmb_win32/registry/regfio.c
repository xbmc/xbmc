/*
 * Unix SMB/CIFS implementation.
 * Windows NT registry I/O library
 * Copyright (c) Gerald (Jerry) Carter               2005
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
 */

#include "includes.h"
#include "regfio.h"

/*******************************************************************
 *
 * TODO : Right now this code basically ignores classnames.
 *
 ******************************************************************/


/*******************************************************************
*******************************************************************/

static int write_block( REGF_FILE *file, prs_struct *ps, uint32 offset )
{
	int bytes_written, returned;
	char *buffer = prs_data_p( ps );
	uint32 buffer_size = prs_data_size( ps );
	SMB_STRUCT_STAT sbuf;

	if ( file->fd == -1 )
		return -1;

	/* check for end of file */

	if ( sys_fstat( file->fd, &sbuf ) ) {
		DEBUG(0,("write_block: stat() failed! (%s)\n", strerror(errno)));
		return -1;
	}

	if ( lseek( file->fd, offset, SEEK_SET ) == -1 ) {
		DEBUG(0,("write_block: lseek() failed! (%s)\n", strerror(errno) ));
		return -1;
	}
	
	bytes_written = returned = 0;
	while ( bytes_written < buffer_size ) {
		if ( (returned = write( file->fd, buffer+bytes_written, buffer_size-bytes_written )) == -1 ) {
			DEBUG(0,("write_block: write() failed! (%s)\n", strerror(errno) ));
			return False;
		}
				
		bytes_written += returned;
	}
	
	return bytes_written;
}

/*******************************************************************
*******************************************************************/

static int read_block( REGF_FILE *file, prs_struct *ps, uint32 file_offset, uint32 block_size )
{
	int bytes_read, returned;
	char *buffer;
	SMB_STRUCT_STAT sbuf;

	/* check for end of file */

	if ( sys_fstat( file->fd, &sbuf ) ) {
		DEBUG(0,("read_block: stat() failed! (%s)\n", strerror(errno)));
		return -1;
	}

	if ( (size_t)file_offset >= sbuf.st_size )
		return -1;
	
	/* if block_size == 0, we are parsing HBIN records and need 
	   to read some of the header to get the block_size from there */
	   
	if ( block_size == 0 ) {
		char hdr[0x20];

		if ( lseek( file->fd, file_offset, SEEK_SET ) == -1 ) {
			DEBUG(0,("read_block: lseek() failed! (%s)\n", strerror(errno) ));
			return -1;
		}

		returned = read( file->fd, hdr, 0x20 );
		if ( (returned == -1) || (returned < 0x20) ) {
			DEBUG(0,("read_block: failed to read in HBIN header. Is the file corrupt?\n"));
			return -1;
		}

		/* make sure this is an hbin header */

		if ( strncmp( hdr, "hbin", HBIN_HDR_SIZE ) != 0 ) {
			DEBUG(0,("read_block: invalid block header!\n"));
			return -1;
		}

		block_size = IVAL( hdr, 0x08 );
	}

	DEBUG(10,("read_block: block_size == 0x%x\n", block_size ));

	/* set the offset, initialize the buffer, and read the block from disk */

	if ( lseek( file->fd, file_offset, SEEK_SET ) == -1 ) {
		DEBUG(0,("read_block: lseek() failed! (%s)\n", strerror(errno) ));
		return -1;
	}
	
	prs_init( ps, block_size, file->mem_ctx, UNMARSHALL );
	buffer = prs_data_p( ps );
	bytes_read = returned = 0;

	while ( bytes_read < block_size ) {
		if ( (returned = read( file->fd, buffer+bytes_read, block_size-bytes_read )) == -1 ) {
			DEBUG(0,("read_block: read() failed (%s)\n", strerror(errno) ));
			return False;
		}
		if ( (returned == 0) && (bytes_read < block_size) ) {
			DEBUG(0,("read_block: not a vald registry file ?\n" ));
			return False;
		}	
		
		bytes_read += returned;
	}
	
	return bytes_read;
}

/*******************************************************************
*******************************************************************/

static BOOL write_hbin_block( REGF_FILE *file, REGF_HBIN *hbin )
{
	if ( !hbin->dirty )
		return True;

	/* write free space record if any is available */

	if ( hbin->free_off != REGF_OFFSET_NONE ) {
		uint32 header = 0xffffffff;

		if ( !prs_set_offset( &hbin->ps, hbin->free_off-sizeof(uint32) ) )
			return False;
		if ( !prs_uint32( "free_size", &hbin->ps, 0, &hbin->free_size ) )
			return False;
		if ( !prs_uint32( "free_header", &hbin->ps, 0, &header ) )
			return False;
	}

	hbin->dirty = (write_block( file, &hbin->ps, hbin->file_off ) != -1);

	return hbin->dirty;
}

/*******************************************************************
*******************************************************************/

static BOOL hbin_block_close( REGF_FILE *file, REGF_HBIN *hbin )
{
	REGF_HBIN *p;

	/* remove the block from the open list and flush it to disk */

	for ( p=file->block_list; p && p!=hbin; p=p->next )
		;;

	if ( p == hbin ) {
		DLIST_REMOVE( file->block_list, hbin );
	}
	else
		DEBUG(0,("hbin_block_close: block not in open list!\n"));

	if ( !write_hbin_block( file, hbin ) )
		return False;

	return True;
}

/*******************************************************************
*******************************************************************/

static BOOL prs_regf_block( const char *desc, prs_struct *ps, int depth, REGF_FILE *file )
{
	prs_debug(ps, depth, desc, "prs_regf_block");
	depth++;
	
	if ( !prs_uint8s( True, "header", ps, depth, (uint8*)file->header, sizeof( file->header )) )
		return False;
	
	/* yes, these values are always identical so store them only once */
	
	if ( !prs_uint32( "unknown1", ps, depth, &file->unknown1 ))
		return False;
	if ( !prs_uint32( "unknown1 (again)", ps, depth, &file->unknown1 ))
		return False;

	/* get the modtime */
	
	if ( !prs_set_offset( ps, 0x0c ) )
		return False;
	if ( !smb_io_time( "modtime", &file->mtime, ps, depth ) )
		return False;

	/* constants */
	
	if ( !prs_uint32( "unknown2", ps, depth, &file->unknown2 ))
		return False;
	if ( !prs_uint32( "unknown3", ps, depth, &file->unknown3 ))
		return False;
	if ( !prs_uint32( "unknown4", ps, depth, &file->unknown4 ))
		return False;
	if ( !prs_uint32( "unknown5", ps, depth, &file->unknown5 ))
		return False;

	/* get file offsets */
	
	if ( !prs_set_offset( ps, 0x24 ) )
		return False;
	if ( !prs_uint32( "data_offset", ps, depth, &file->data_offset ))
		return False;
	if ( !prs_uint32( "last_block", ps, depth, &file->last_block ))
		return False;
		
	/* one more constant */
	
	if ( !prs_uint32( "unknown6", ps, depth, &file->unknown6 ))
		return False;
		
	/* get the checksum */
	
	if ( !prs_set_offset( ps, 0x01fc ) )
		return False;
	if ( !prs_uint32( "checksum", ps, depth, &file->checksum ))
		return False;
	
	return True;
}

/*******************************************************************
*******************************************************************/

static BOOL prs_hbin_block( const char *desc, prs_struct *ps, int depth, REGF_HBIN *hbin )
{
	uint32 block_size2;

	prs_debug(ps, depth, desc, "prs_regf_block");
	depth++;
	
	if ( !prs_uint8s( True, "header", ps, depth, (uint8*)hbin->header, sizeof( hbin->header )) )
		return False;

	if ( !prs_uint32( "first_hbin_off", ps, depth, &hbin->first_hbin_off ))
		return False;

	/* The dosreg.cpp comments say that the block size is at 0x1c.
	   According to a WINXP NTUSER.dat file, this is wrong.  The block_size
	   is at 0x08 */

	if ( !prs_uint32( "block_size", ps, depth, &hbin->block_size ))
		return False;

	block_size2 = hbin->block_size;
	prs_set_offset( ps, 0x1c );
	if ( !prs_uint32( "block_size2", ps, depth, &block_size2 ))
		return False;

	if ( MARSHALLING(ps) )
		hbin->dirty = True;
	

	return True;
}

/*******************************************************************
*******************************************************************/

static BOOL prs_nk_rec( const char *desc, prs_struct *ps, int depth, REGF_NK_REC *nk )
{
	uint16 class_length, name_length;
	uint32 start;
	uint32 data_size, start_off, end_off;
	uint32 unknown_off = REGF_OFFSET_NONE;

	nk->hbin_off = prs_offset( ps );
	start = nk->hbin_off;
	
	prs_debug(ps, depth, desc, "prs_nk_rec");
	depth++;
	
	/* back up and get the data_size */
	
	if ( !prs_set_offset( ps, prs_offset(ps)-sizeof(uint32)) )
		return False;
	start_off = prs_offset( ps );
	if ( !prs_uint32( "rec_size", ps, depth, &nk->rec_size ))
		return False;
	
	if ( !prs_uint8s( True, "header", ps, depth, (uint8*)nk->header, sizeof( nk->header )) )
		return False;
		
	if ( !prs_uint16( "key_type", ps, depth, &nk->key_type ))
		return False;
	if ( !smb_io_time( "mtime", &nk->mtime, ps, depth ))
		return False;
		
	if ( !prs_set_offset( ps, start+0x0010 ) )
		return False;
	if ( !prs_uint32( "parent_off", ps, depth, &nk->parent_off ))
		return False;
	if ( !prs_uint32( "num_subkeys", ps, depth, &nk->num_subkeys ))
		return False;
		
	if ( !prs_set_offset( ps, start+0x001c ) )
		return False;
	if ( !prs_uint32( "subkeys_off", ps, depth, &nk->subkeys_off ))
		return False;
	if ( !prs_uint32( "unknown_off", ps, depth, &unknown_off) )
		return False;
		
	if ( !prs_set_offset( ps, start+0x0024 ) )
		return False;
	if ( !prs_uint32( "num_values", ps, depth, &nk->num_values ))
		return False;
	if ( !prs_uint32( "values_off", ps, depth, &nk->values_off ))
		return False;
	if ( !prs_uint32( "sk_off", ps, depth, &nk->sk_off ))
		return False;
	if ( !prs_uint32( "classname_off", ps, depth, &nk->classname_off ))
		return False;

	if ( !prs_uint32( "max_bytes_subkeyname", ps, depth, &nk->max_bytes_subkeyname))
		return False;
	if ( !prs_uint32( "max_bytes_subkeyclassname", ps, depth, &nk->max_bytes_subkeyclassname))
		return False;
	if ( !prs_uint32( "max_bytes_valuename", ps, depth, &nk->max_bytes_valuename))
		return False;
	if ( !prs_uint32( "max_bytes_value", ps, depth, &nk->max_bytes_value))
		return False;
	if ( !prs_uint32( "unknown index", ps, depth, &nk->unk_index))
		return False;

	name_length = nk->keyname ? strlen(nk->keyname) : 0 ;
	class_length = nk->classname ? strlen(nk->classname) : 0 ;
	if ( !prs_uint16( "name_length", ps, depth, &name_length ))
		return False;
	if ( !prs_uint16( "class_length", ps, depth, &class_length ))
		return False;	
		
	if ( class_length ) {
		;;
	}
	
	if ( name_length ) {
		if ( UNMARSHALLING(ps) ) {
			if ( !(nk->keyname = PRS_ALLOC_MEM( ps, char, name_length+1 )) )
				return False;
		}

		if ( !prs_uint8s( True, "name", ps, depth, (uint8*)nk->keyname, name_length) )
			return False;

		if ( UNMARSHALLING(ps) ) 
			nk->keyname[name_length] = '\0';
	}

	end_off = prs_offset( ps );

	/* data_size must be divisible by 8 and large enough to hold the original record */

	data_size = ((start_off - end_off) & 0xfffffff8 );
	if ( data_size > nk->rec_size )
		DEBUG(10,("Encountered reused record (0x%x < 0x%x)\n", data_size, nk->rec_size));

	if ( MARSHALLING(ps) )
		nk->hbin->dirty = True;

	return True;
}

/*******************************************************************
*******************************************************************/

static uint32 regf_block_checksum( prs_struct *ps )
{
	char *buffer = prs_data_p( ps );
	uint32 checksum, x;
	int i;

	/* XOR of all bytes 0x0000 - 0x01FB */
		
	checksum = x = 0;
	
	for ( i=0; i<0x01FB; i+=4 ) {
		x = IVAL(buffer, i );
		checksum ^= x;
	}
	
	return checksum;
}

/*******************************************************************
*******************************************************************/

static BOOL read_regf_block( REGF_FILE *file )
{
	prs_struct ps;
	uint32 checksum;
	
	/* grab the first block from the file */
		
	if ( read_block( file, &ps, 0, REGF_BLOCKSIZE ) == -1 )
		return False;
	
	/* parse the block and verify the checksum */
	
	if ( !prs_regf_block( "regf_header", &ps, 0, file ) )
		return False;	
		
	checksum = regf_block_checksum( &ps );
	
	prs_mem_free( &ps );
	
	if ( file->checksum !=  checksum ) {
		DEBUG(0,("read_regf_block: invalid checksum\n" ));
		return False;
	}

	return True;
}

/*******************************************************************
*******************************************************************/

static REGF_HBIN* read_hbin_block( REGF_FILE *file, off_t offset )
{
	REGF_HBIN *hbin;
	uint32 record_size, curr_off, block_size, header;
	
	if ( !(hbin = TALLOC_ZERO_P(file->mem_ctx, REGF_HBIN)) ) 
		return NULL;
	hbin->file_off = offset;
	hbin->free_off = -1;
		
	if ( read_block( file, &hbin->ps, offset, 0 ) == -1 )
		return NULL;
	
	if ( !prs_hbin_block( "hbin", &hbin->ps, 0, hbin ) )
		return NULL;	

	/* this should be the same thing as hbin->block_size but just in case */

	block_size = prs_data_size( &hbin->ps );	

	/* Find the available free space offset.  Always at the end,
	   so walk the record list and stop when you get to the end.
	   The end is defined by a record header of 0xffffffff.  The 
	   previous 4 bytes contains the amount of free space remaining 
	   in the hbin block. */

	/* remember that the record_size is in the 4 bytes preceeding the record itself */

	if ( !prs_set_offset( &hbin->ps, file->data_offset+HBIN_HDR_SIZE-sizeof(uint32) ) )
		return False;

	record_size = 0;
	header = 0;
	curr_off = prs_offset( &hbin->ps );
	while ( header != 0xffffffff ) {
		/* not done yet so reset the current offset to the 
		   next record_size field */

		curr_off = curr_off+record_size;

		/* for some reason the record_size of the last record in
		   an hbin block can extend past the end of the block
		   even though the record fits within the remaining 
		   space....aaarrrgggghhhhhh */

		if ( curr_off >= block_size ) {
			record_size = -1;
			curr_off = -1;
			break;
		}

		if ( !prs_set_offset( &hbin->ps, curr_off) )
			return False;

		if ( !prs_uint32( "rec_size", &hbin->ps, 0, &record_size ) )
			return False;
		if ( !prs_uint32( "header", &hbin->ps, 0, &header ) )
			return False;
		
		SMB_ASSERT( record_size != 0 );

		if ( record_size & 0x80000000 ) {
			/* absolute_value(record_size) */
			record_size = (record_size ^ 0xffffffff) + 1;
		}
	}

	/* save the free space offset */

	if ( header == 0xffffffff ) {

		/* account for the fact that the curr_off is 4 bytes behind the actual 
		   record header */

		hbin->free_off = curr_off + sizeof(uint32);
		hbin->free_size = record_size;
	}

	DEBUG(10,("read_hbin_block: free space offset == 0x%x\n", hbin->free_off));

	if ( !prs_set_offset( &hbin->ps, file->data_offset+HBIN_HDR_SIZE )  )
		return False;
	
	return hbin;
}

/*******************************************************************
 Input a random offset and receive the corresponding HBIN 
 block for it
*******************************************************************/

static BOOL hbin_contains_offset( REGF_HBIN *hbin, uint32 offset )
{
	if ( !hbin )
		return False;
	
	if ( (offset > hbin->first_hbin_off) && (offset < (hbin->first_hbin_off+hbin->block_size)) )
		return True;
		
	return False;
}

/*******************************************************************
 Input a random offset and receive the corresponding HBIN 
 block for it
*******************************************************************/

static REGF_HBIN* lookup_hbin_block( REGF_FILE *file, uint32 offset )
{
	REGF_HBIN *hbin = NULL;
	uint32 block_off;

	/* start with the open list */

	for ( hbin=file->block_list; hbin; hbin=hbin->next ) {
		DEBUG(10,("lookup_hbin_block: address = 0x%x [0x%lx]\n", hbin->file_off, (unsigned long)hbin ));
		if ( hbin_contains_offset( hbin, offset ) )
			return hbin;
	}
	
	if ( !hbin ) {
		/* start at the beginning */

		block_off = REGF_BLOCKSIZE;
		do {
			/* cleanup before the next round */
			if ( hbin )
				prs_mem_free( &hbin->ps );

			hbin = read_hbin_block( file, block_off );

			if ( hbin ) 
				block_off = hbin->file_off + hbin->block_size;

		} while ( hbin && !hbin_contains_offset( hbin, offset ) );
	}

	if ( hbin )
		DLIST_ADD( file->block_list, hbin );

	return hbin;
}

/*******************************************************************
*******************************************************************/

static BOOL prs_hash_rec( const char *desc, prs_struct *ps, int depth, REGF_HASH_REC *hash )
{
	prs_debug(ps, depth, desc, "prs_hash_rec");
	depth++;

	if ( !prs_uint32( "nk_off", ps, depth, &hash->nk_off ))
		return False;
	if ( !prs_uint8s( True, "keycheck", ps, depth, hash->keycheck, sizeof( hash->keycheck )) )
		return False;
	
	return True;
}

/*******************************************************************
*******************************************************************/

static BOOL hbin_prs_lf_records( const char *desc, REGF_HBIN *hbin, int depth, REGF_NK_REC *nk )
{
	int i;
	REGF_LF_REC *lf = &nk->subkeys;
	uint32 data_size, start_off, end_off;

	prs_debug(&hbin->ps, depth, desc, "prs_lf_records");
	depth++;

	/* check if we have anything to do first */
	
	if ( nk->num_subkeys == 0 )
		return True;

	/* move to the LF record */

	if ( !prs_set_offset( &hbin->ps, nk->subkeys_off + HBIN_HDR_SIZE - hbin->first_hbin_off ) )
		return False;

	/* backup and get the data_size */
	
	if ( !prs_set_offset( &hbin->ps, prs_offset(&hbin->ps)-sizeof(uint32)) )
		return False;
	start_off = prs_offset( &hbin->ps );
	if ( !prs_uint32( "rec_size", &hbin->ps, depth, &lf->rec_size ))
		return False;

	if ( !prs_uint8s( True, "header", &hbin->ps, depth, (uint8*)lf->header, sizeof( lf->header )) )
		return False;
		
	if ( !prs_uint16( "num_keys", &hbin->ps, depth, &lf->num_keys))
		return False;

	if ( UNMARSHALLING(&hbin->ps) ) {
		if ( !(lf->hashes = PRS_ALLOC_MEM( &hbin->ps, REGF_HASH_REC, lf->num_keys )) )
			return False;
	}

	for ( i=0; i<lf->num_keys; i++ ) {
		if ( !prs_hash_rec( "hash_rec", &hbin->ps, depth, &lf->hashes[i] ) )
			return False;
	}

	end_off = prs_offset( &hbin->ps );

	/* data_size must be divisible by 8 and large enough to hold the original record */

	data_size = ((start_off - end_off) & 0xfffffff8 );
	if ( data_size > lf->rec_size )
		DEBUG(10,("Encountered reused record (0x%x < 0x%x)\n", data_size, lf->rec_size));

	if ( MARSHALLING(&hbin->ps) )
		hbin->dirty = True;

	return True;
}

/*******************************************************************
*******************************************************************/

static BOOL hbin_prs_sk_rec( const char *desc, REGF_HBIN *hbin, int depth, REGF_SK_REC *sk )
{
	prs_struct *ps = &hbin->ps;
	uint16 tag = 0xFFFF;
	uint32 data_size, start_off, end_off;


	prs_debug(ps, depth, desc, "hbin_prs_sk_rec");
	depth++;

	if ( !prs_set_offset( &hbin->ps, sk->sk_off + HBIN_HDR_SIZE - hbin->first_hbin_off ) )
		return False;

	/* backup and get the data_size */
	
	if ( !prs_set_offset( &hbin->ps, prs_offset(&hbin->ps)-sizeof(uint32)) )
		return False;
	start_off = prs_offset( &hbin->ps );
	if ( !prs_uint32( "rec_size", &hbin->ps, depth, &sk->rec_size ))
		return False;

	if ( !prs_uint8s( True, "header", ps, depth, (uint8*)sk->header, sizeof( sk->header )) )
		return False;
	if ( !prs_uint16( "tag", ps, depth, &tag))
		return False;

	if ( !prs_uint32( "prev_sk_off", ps, depth, &sk->prev_sk_off))
		return False;
	if ( !prs_uint32( "next_sk_off", ps, depth, &sk->next_sk_off))
		return False;
	if ( !prs_uint32( "ref_count", ps, depth, &sk->ref_count))
		return False;
	if ( !prs_uint32( "size", ps, depth, &sk->size))
		return False;

	if ( !sec_io_desc( "sec_desc", &sk->sec_desc, ps, depth )) 
		return False;

	end_off = prs_offset( &hbin->ps );

	/* data_size must be divisible by 8 and large enough to hold the original record */

	data_size = ((start_off - end_off) & 0xfffffff8 );
	if ( data_size > sk->rec_size )
		DEBUG(10,("Encountered reused record (0x%x < 0x%x)\n", data_size, sk->rec_size));

	if ( MARSHALLING(&hbin->ps) )
		hbin->dirty = True;

	return True;
}

/*******************************************************************
*******************************************************************/

static BOOL hbin_prs_vk_rec( const char *desc, REGF_HBIN *hbin, int depth, REGF_VK_REC *vk, REGF_FILE *file )
{
	uint32 offset;
	uint16 name_length;
	prs_struct *ps = &hbin->ps;
	uint32 data_size, start_off, end_off;

	prs_debug(ps, depth, desc, "prs_vk_rec");
	depth++;

	/* backup and get the data_size */
	
	if ( !prs_set_offset( &hbin->ps, prs_offset(&hbin->ps)-sizeof(uint32)) )
		return False;
	start_off = prs_offset( &hbin->ps );
	if ( !prs_uint32( "rec_size", &hbin->ps, depth, &vk->rec_size ))
		return False;

	if ( !prs_uint8s( True, "header", ps, depth, (uint8*)vk->header, sizeof( vk->header )) )
		return False;

	if ( MARSHALLING(&hbin->ps) )
		name_length = strlen(vk->valuename);

	if ( !prs_uint16( "name_length", ps, depth, &name_length ))
		return False;
	if ( !prs_uint32( "data_size", ps, depth, &vk->data_size ))
		return False;
	if ( !prs_uint32( "data_off", ps, depth, &vk->data_off ))
		return False;
	if ( !prs_uint32( "type", ps, depth, &vk->type))
		return False;
	if ( !prs_uint16( "flag", ps, depth, &vk->flag))
		return False;

	offset = prs_offset( ps );
	offset += 2;	/* skip 2 bytes */
	prs_set_offset( ps, offset );

	/* get the name */

	if ( vk->flag&VK_FLAG_NAME_PRESENT ) {

		if ( UNMARSHALLING(&hbin->ps) ) {
			if ( !(vk->valuename = PRS_ALLOC_MEM( ps, char, name_length+1 )))
				return False;
		}
		if ( !prs_uint8s( True, "name", ps, depth, (uint8*)vk->valuename, name_length ) )
			return False;
	}

	end_off = prs_offset( &hbin->ps );

	/* get the data if necessary */

	if ( vk->data_size != 0 ) {
		BOOL charmode = False;

		if ( (vk->type == REG_SZ) || (vk->type == REG_MULTI_SZ) )
			charmode = True;

		/* the data is stored in the offset if the size <= 4 */

		if ( !(vk->data_size & VK_DATA_IN_OFFSET) ) {
			REGF_HBIN *hblock = hbin;
			uint32 data_rec_size;

			if ( UNMARSHALLING(&hbin->ps) ) {
				if ( !(vk->data = PRS_ALLOC_MEM( ps, uint8, vk->data_size) ) )
					return False;
			}

			/* this data can be in another hbin */
			if ( !hbin_contains_offset( hbin, vk->data_off ) ) {
				if ( !(hblock = lookup_hbin_block( file, vk->data_off )) )
					return False;
			}
			if ( !(prs_set_offset( &hblock->ps, (vk->data_off+HBIN_HDR_SIZE-hblock->first_hbin_off)-sizeof(uint32) )) )
				return False;

			if ( MARSHALLING(&hblock->ps) ) {
				data_rec_size = ( (vk->data_size+sizeof(uint32)) & 0xfffffff8 ) + 8;
				data_rec_size = ( data_rec_size - 1 ) ^ 0xFFFFFFFF;
			}
			if ( !prs_uint32( "data_rec_size", &hblock->ps, depth, &data_rec_size ))
				return False;
			if ( !prs_uint8s( charmode, "data", &hblock->ps, depth, vk->data, vk->data_size) )
				return False;

			if ( MARSHALLING(&hblock->ps) )
				hblock->dirty = True;
		}
		else {
			if ( !(vk->data = PRS_ALLOC_MEM( ps, uint8, 4 ) ) )
				return False;
			SIVAL( vk->data, 0, vk->data_off );
		}
		
	}

	/* data_size must be divisible by 8 and large enough to hold the original record */

	data_size = ((start_off - end_off ) & 0xfffffff8 );
	if ( data_size !=  vk->rec_size )
		DEBUG(10,("prs_vk_rec: data_size check failed (0x%x < 0x%x)\n", data_size, vk->rec_size));

	if ( MARSHALLING(&hbin->ps) )
		hbin->dirty = True;

	return True;
}

/*******************************************************************
 read a VK record which is contained in the HBIN block stored 
 in the prs_struct *ps.
*******************************************************************/

static BOOL hbin_prs_vk_records( const char *desc, REGF_HBIN *hbin, int depth, REGF_NK_REC *nk, REGF_FILE *file )
{
	int i;
	uint32 record_size;

	prs_debug(&hbin->ps, depth, desc, "prs_vk_records");
	depth++;
	
	/* check if we have anything to do first */
	
	if ( nk->num_values == 0 )
		return True;
		
	if ( UNMARSHALLING(&hbin->ps) ) {
		if ( !(nk->values = PRS_ALLOC_MEM( &hbin->ps, REGF_VK_REC, nk->num_values ) ) )
			return False;
	}
	
	/* convert the offset to something relative to this HBIN block */
	
	if ( !prs_set_offset( &hbin->ps, nk->values_off+HBIN_HDR_SIZE-hbin->first_hbin_off-sizeof(uint32)) )
		return False;

	if ( MARSHALLING( &hbin->ps) ) { 
		record_size = ( ( nk->num_values * sizeof(uint32) ) & 0xfffffff8 ) + 8;
		record_size = (record_size - 1) ^ 0xFFFFFFFF;
	}

	if ( !prs_uint32( "record_size", &hbin->ps, depth, &record_size ) )
		return False;
		
	for ( i=0; i<nk->num_values; i++ ) {
		if ( !prs_uint32( "vk_off", &hbin->ps, depth, &nk->values[i].rec_off ) )
			return False;
	}

	for ( i=0; i<nk->num_values; i++ ) {
		REGF_HBIN *sub_hbin = hbin;
		uint32 new_offset;
	
		if ( !hbin_contains_offset( hbin, nk->values[i].rec_off ) ) {
			sub_hbin = lookup_hbin_block( file, nk->values[i].rec_off );
			if ( !sub_hbin ) {
				DEBUG(0,("hbin_prs_vk_records: Failed to find HBIN block containing offset [0x%x]\n", 
					nk->values[i].hbin_off));
				return False;
			}
		}
		
		new_offset = nk->values[i].rec_off + HBIN_HDR_SIZE - sub_hbin->first_hbin_off;
		if ( !prs_set_offset( &sub_hbin->ps, new_offset ) )
			return False;
		if ( !hbin_prs_vk_rec( "vk_rec", sub_hbin, depth, &nk->values[i], file ) )
			return False;
	}

	if ( MARSHALLING(&hbin->ps) )
		hbin->dirty = True;


	return True;
}


/*******************************************************************
*******************************************************************/

static REGF_SK_REC* find_sk_record_by_offset( REGF_FILE *file, uint32 offset )
{
	REGF_SK_REC *p_sk;
	
	for ( p_sk=file->sec_desc_list; p_sk; p_sk=p_sk->next ) {
		if ( p_sk->sk_off == offset ) 
			return p_sk;
	}
	
	return NULL;
}

/*******************************************************************
*******************************************************************/

static REGF_SK_REC* find_sk_record_by_sec_desc( REGF_FILE *file, SEC_DESC *sd )
{
	REGF_SK_REC *p;

	for ( p=file->sec_desc_list; p; p=p->next ) {
		if ( sec_desc_equal( p->sec_desc, sd ) )
			return p;
	}

	/* failure */

	return NULL;
}

/*******************************************************************
*******************************************************************/

static BOOL hbin_prs_key( REGF_FILE *file, REGF_HBIN *hbin, REGF_NK_REC *nk )
{
	int depth = 0;
	REGF_HBIN *sub_hbin;
	
	prs_debug(&hbin->ps, depth, "", "fetch_key");
	depth++;

	/* get the initial nk record */
	
	if ( !prs_nk_rec( "nk_rec", &hbin->ps, depth, nk ))
		return False;

	/* fill in values */
	
	if ( nk->num_values && (nk->values_off!=REGF_OFFSET_NONE) ) {
		sub_hbin = hbin;
		if ( !hbin_contains_offset( hbin, nk->values_off ) ) {
			sub_hbin = lookup_hbin_block( file, nk->values_off );
			if ( !sub_hbin ) {
				DEBUG(0,("hbin_prs_key: Failed to find HBIN block containing value_list_offset [0x%x]\n", 
					nk->values_off));
				return False;
			}
		}
		
		if ( !hbin_prs_vk_records( "vk_rec", sub_hbin, depth, nk, file ))
			return False;
	}
		
	/* now get subkeys */
	
	if ( nk->num_subkeys && (nk->subkeys_off!=REGF_OFFSET_NONE) ) {
		sub_hbin = hbin;
		if ( !hbin_contains_offset( hbin, nk->subkeys_off ) ) {
			sub_hbin = lookup_hbin_block( file, nk->subkeys_off );
			if ( !sub_hbin ) {
				DEBUG(0,("hbin_prs_key: Failed to find HBIN block containing subkey_offset [0x%x]\n", 
					nk->subkeys_off));
				return False;
			}
		}
		
		if ( !hbin_prs_lf_records( "lf_rec", sub_hbin, depth, nk ))
			return False;
	}

	/* get the to the security descriptor.  First look if we have already parsed it */
	
	if ( (nk->sk_off!=REGF_OFFSET_NONE) && !( nk->sec_desc = find_sk_record_by_offset( file, nk->sk_off )) ) {

		sub_hbin = hbin;
		if ( !hbin_contains_offset( hbin, nk->sk_off ) ) {
			sub_hbin = lookup_hbin_block( file, nk->sk_off );
			if ( !sub_hbin ) {
				DEBUG(0,("hbin_prs_key: Failed to find HBIN block containing sk_offset [0x%x]\n", 
					nk->subkeys_off));
				return False;
			}
		}
		
		if ( !(nk->sec_desc = TALLOC_ZERO_P( file->mem_ctx, REGF_SK_REC )) )
			return False;
		nk->sec_desc->sk_off = nk->sk_off;
		if ( !hbin_prs_sk_rec( "sk_rec", sub_hbin, depth, nk->sec_desc ))
			return False;
			
		/* add to the list of security descriptors (ref_count has been read from the files) */

		nk->sec_desc->sk_off = nk->sk_off;
		DLIST_ADD( file->sec_desc_list, nk->sec_desc );
	}
		
	return True;
}

/*******************************************************************
*******************************************************************/

static BOOL next_record( REGF_HBIN *hbin, const char *hdr, BOOL *eob )
{
	uint8 header[REC_HDR_SIZE];
	uint32 record_size;
	uint32 curr_off, block_size;
	BOOL found = False;
	prs_struct *ps = &hbin->ps;
	
	curr_off = prs_offset( ps );
	if ( curr_off == 0 )
		prs_set_offset( ps, HBIN_HEADER_REC_SIZE );

	/* assume that the current offset is at the record header 
	   and we need to backup to read the record size */

	curr_off -= sizeof(uint32);

	block_size = prs_data_size( ps );
	record_size = 0;
	memset( header, 0x0, sizeof(uint8)*REC_HDR_SIZE );
	while ( !found ) {

		curr_off = curr_off+record_size;
		if ( curr_off >= block_size ) 
			break;

		if ( !prs_set_offset( &hbin->ps, curr_off) )
			return False;

		if ( !prs_uint32( "record_size", ps, 0, &record_size ) )
			return False;
		if ( !prs_uint8s( True, "header", ps, 0, header, REC_HDR_SIZE ) )
			return False;

		if ( record_size & 0x80000000 ) {
			/* absolute_value(record_size) */
			record_size = (record_size ^ 0xffffffff) + 1;
		}

		if ( memcmp( header, hdr, REC_HDR_SIZE ) == 0 ) {
			found = True;
			curr_off += sizeof(uint32);
		}
	} 

	/* mark prs_struct as done ( at end ) if no more SK records */
	/* mark end-of-block as True */
	
	if ( !found ) {
		prs_set_offset( &hbin->ps, prs_data_size(&hbin->ps) );
		*eob = True;
		return False;
	}
		
	if ( !prs_set_offset( ps, curr_off ) )
		return False;

	return True;
}

/*******************************************************************
*******************************************************************/

static BOOL next_nk_record( REGF_FILE *file, REGF_HBIN *hbin, REGF_NK_REC *nk, BOOL *eob )
{
	if ( next_record( hbin, "nk", eob ) && hbin_prs_key( file, hbin, nk ) )
		return True;
	
	return False;
}

/*******************************************************************
 Intialize the newly created REGF_BLOCK in *file and write the 
 block header to disk 
*******************************************************************/

static BOOL init_regf_block( REGF_FILE *file )
{	
	prs_struct ps;
	BOOL result = True;
	
	if ( !prs_init( &ps, REGF_BLOCKSIZE, file->mem_ctx, MARSHALL ) )
		return False;
		
	memcpy( file->header, "regf", REGF_HDR_SIZE );
	file->data_offset = 0x20;
	file->last_block  = 0x1000;
	
	/* set mod time */
	
	unix_to_nt_time( &file->mtime, time(NULL) );
	
	/* hard coded values...no diea what these are ... maybe in time */
	
	file->unknown1 = 0x2;
	file->unknown2 = 0x1;
	file->unknown3 = 0x3;
	file->unknown4 = 0x0;
	file->unknown5 = 0x1;
	file->unknown6 = 0x1;
	
	/* write header to the buffer */
	
	if ( !prs_regf_block( "regf_header", &ps, 0, file ) ) {
		result = False;
		goto out;
	}
	
	/* calculate the checksum, re-marshall data (to include the checksum) 
	   and write to disk */
	
	file->checksum = regf_block_checksum( &ps );
	prs_set_offset( &ps, 0 );
	if ( !prs_regf_block( "regf_header", &ps, 0, file ) ) {
		result = False;
		goto out;
	}
		
	if ( write_block( file, &ps, 0 ) == -1 ) {
		DEBUG(0,("init_regf_block: Failed to initialize registry header block!\n"));
		result = False;
		goto out;
	}
	
out:
	prs_mem_free( &ps );

	return result;
}
/*******************************************************************
 Open the registry file and then read in the REGF block to get the 
 first hbin offset.
*******************************************************************/

 REGF_FILE* regfio_open( const char *filename, int flags, int mode )
{
	REGF_FILE *rb;
	
	if ( !(rb = SMB_MALLOC_P(REGF_FILE)) ) {
		DEBUG(0,("ERROR allocating memory\n"));
		return NULL;
	}
	ZERO_STRUCTP( rb );
	rb->fd = -1;
	
	if ( !(rb->mem_ctx = talloc_init( "read_regf_block" )) ) {
		regfio_close( rb );
		return NULL;
	}

	rb->open_flags = flags;
	
	/* open and existing file */

	if ( (rb->fd = open(filename, flags, mode)) == -1 ) {
		DEBUG(0,("regfio_open: failure to open %s (%s)\n", filename, strerror(errno)));
		regfio_close( rb );
		return NULL;
	}
	
	/* check if we are creating a new file or overwriting an existing one */
		
	if ( flags & (O_CREAT|O_TRUNC) ) {
		if ( !init_regf_block( rb ) ) {
			DEBUG(0,("regfio_open: Failed to read initial REGF block\n"));
			regfio_close( rb );
			return NULL;
		}
		
		/* success */
		return rb;
	}
	
	/* read in an existing file */
	
	if ( !read_regf_block( rb ) ) {
		DEBUG(0,("regfio_open: Failed to read initial REGF block\n"));
		regfio_close( rb );
		return NULL;
	}
	
	/* success */
	
	return rb;
}

/*******************************************************************
*******************************************************************/

static void regfio_mem_free( REGF_FILE *file )
{
	/* free any talloc()'d memory */
	
	if ( file && file->mem_ctx )
		talloc_destroy( file->mem_ctx );	
}

/*******************************************************************
*******************************************************************/

 int regfio_close( REGF_FILE *file )
{
	int fd;

	/* cleanup for a file opened for write */

	if ( file->open_flags & (O_WRONLY|O_RDWR) ) {
		prs_struct ps;
		REGF_SK_REC *sk;

		/* write of sd list */

		for ( sk=file->sec_desc_list; sk; sk=sk->next ) {
			hbin_prs_sk_rec( "sk_rec", sk->hbin, 0, sk );
		}

		/* flush any dirty blocks */

		while ( file->block_list ) {
			hbin_block_close( file, file->block_list );
		} 

		ZERO_STRUCT( ps );

		unix_to_nt_time( &file->mtime, time(NULL) );

		if ( read_block( file, &ps, 0, REGF_BLOCKSIZE ) != -1 ) {
			/* now use for writing */
			prs_switch_type( &ps, MARSHALL );

			/* stream the block once, generate the checksum, 
			   and stream it again */
			prs_set_offset( &ps, 0 );
			prs_regf_block( "regf_blocK", &ps, 0, file );
			file->checksum = regf_block_checksum( &ps );
			prs_set_offset( &ps, 0 );
			prs_regf_block( "regf_blocK", &ps, 0, file );

			/* now we are ready to write it to disk */
			if ( write_block( file, &ps, 0 ) == -1 )
				DEBUG(0,("regfio_close: failed to update the regf header block!\n"));
		}

		prs_mem_free( &ps );
	}
	
	regfio_mem_free( file );

	/* nothing tdo do if there is no open file */

	if ( !file || (file->fd == -1) )
		return 0;
		
	fd = file->fd;
	file->fd = -1;
	SAFE_FREE( file );

	return close( fd );
}

/*******************************************************************
*******************************************************************/

static void regfio_flush( REGF_FILE *file )
{
	REGF_HBIN *hbin;

	for ( hbin=file->block_list; hbin; hbin=hbin->next ) {
		write_hbin_block( file, hbin );
	}
}

/*******************************************************************
 There should be only *one* root key in the registry file based 
 on my experience.  --jerry
*******************************************************************/

REGF_NK_REC* regfio_rootkey( REGF_FILE *file )
{
	REGF_NK_REC *nk;
	REGF_HBIN   *hbin;
	uint32      offset = REGF_BLOCKSIZE;
	BOOL        found = False;
	BOOL        eob;
	
	if ( !file )
		return NULL;
		
	if ( !(nk = TALLOC_ZERO_P( file->mem_ctx, REGF_NK_REC )) ) {
		DEBUG(0,("regfio_rootkey: talloc() failed!\n"));
		return NULL;
	}
	
	/* scan through the file on HBIN block at a time looking 
	   for an NK record with a type == 0x002c.
	   Normally this is the first nk record in the first hbin 
	   block (but I'm not assuming that for now) */
	
	while ( (hbin = read_hbin_block( file, offset )) ) {
		eob = False;

		while ( !eob) {
			if ( next_nk_record( file, hbin, nk, &eob ) ) {
				if ( nk->key_type == NK_TYPE_ROOTKEY ) {
					found = True;
					break;
				}
			}
			prs_mem_free( &hbin->ps );
		}
		
		if ( found ) 
			break;

		offset += hbin->block_size;
	}
	
	if ( !found ) {
		DEBUG(0,("regfio_rootkey: corrupt registry file ?  No root key record located\n"));
		return NULL;
	}

	DLIST_ADD( file->block_list, hbin );

	return nk;		
}

/*******************************************************************
 This acts as an interator over the subkeys defined for a given 
 NK record.  Remember that offsets are from the *first* HBIN block.
*******************************************************************/

 REGF_NK_REC* regfio_fetch_subkey( REGF_FILE *file, REGF_NK_REC *nk )
{
	REGF_NK_REC *subkey;
	REGF_HBIN   *hbin;
	uint32      nk_offset;

	/* see if there is anything left to report */
	
	if ( !nk || (nk->subkeys_off==REGF_OFFSET_NONE) || (nk->subkey_index >= nk->num_subkeys) )
		return NULL;

	/* find the HBIN block which should contain the nk record */
	
	if ( !(hbin = lookup_hbin_block( file, nk->subkeys.hashes[nk->subkey_index].nk_off )) ) {
		DEBUG(0,("hbin_prs_key: Failed to find HBIN block containing offset [0x%x]\n", 
			nk->subkeys.hashes[nk->subkey_index].nk_off));
		return NULL;
	}
	
	nk_offset = nk->subkeys.hashes[nk->subkey_index].nk_off;
	if ( !prs_set_offset( &hbin->ps, (HBIN_HDR_SIZE + nk_offset - hbin->first_hbin_off) ) )
		return NULL;
		
	nk->subkey_index++;
	if ( !(subkey = TALLOC_ZERO_P( file->mem_ctx, REGF_NK_REC )) )
		return NULL;
		
	if ( !hbin_prs_key( file, hbin, subkey ) )
		return NULL;
	
	return subkey;
}


/*******************************************************************
*******************************************************************/

static REGF_HBIN* regf_hbin_allocate( REGF_FILE *file, uint32 block_size )
{
	REGF_HBIN *hbin;
	SMB_STRUCT_STAT sbuf;

	if ( !(hbin = TALLOC_ZERO_P( file->mem_ctx, REGF_HBIN )) )
		return NULL;

	memcpy( hbin->header, "hbin", sizeof(HBIN_HDR_SIZE) );


	if ( sys_fstat( file->fd, &sbuf ) ) {
		DEBUG(0,("regf_hbin_allocate: stat() failed! (%s)\n", strerror(errno)));
		return NULL;
	}

	hbin->file_off       = sbuf.st_size;

	hbin->free_off       = HBIN_HEADER_REC_SIZE;
	hbin->free_size      = block_size - hbin->free_off + sizeof(uint32);;

	hbin->block_size     = block_size;
	hbin->first_hbin_off = hbin->file_off - REGF_BLOCKSIZE;

	if ( !prs_init( &hbin->ps, block_size, file->mem_ctx, MARSHALL ) )
		return NULL;

	if ( !prs_hbin_block( "new_hbin", &hbin->ps, 0, hbin ) )
		return NULL;

	if ( !write_hbin_block( file, hbin ) )
		return NULL;

	file->last_block = hbin->file_off;

	return hbin;
}

/*******************************************************************
*******************************************************************/

static void update_free_space( REGF_HBIN *hbin, uint32 size_used )
{
	hbin->free_off  += size_used;
	hbin->free_size -= size_used;

	if ( hbin->free_off >= hbin->block_size ) {
		hbin->free_off = REGF_OFFSET_NONE;
	}

	return;
}

/*******************************************************************
*******************************************************************/

static REGF_HBIN* find_free_space( REGF_FILE *file, uint32 size )
{
	REGF_HBIN *hbin, *p_hbin;
	uint32 block_off;
	BOOL cached;

	/* check open block list */

	for ( hbin=file->block_list; hbin!=NULL; hbin=hbin->next ) {
		/* only check blocks that actually have available space */

		if ( hbin->free_off == REGF_OFFSET_NONE )
			continue;

		/* check for a large enough available chunk */

		if ( (hbin->block_size - hbin->free_off) >= size ) {
			DLIST_PROMOTE( file->block_list, hbin );
			goto done;			
		}
	}

	/* parse the file until we find a block with 
	   enough free space; save the last non-filled hbin */

	block_off = REGF_BLOCKSIZE;
	do {
		/* cleanup before the next round */
		cached = False;
		if ( hbin )
			prs_mem_free( &hbin->ps );

		hbin = read_hbin_block( file, block_off );

		if ( hbin ) {

			/* make sure that we don't already have this block in memory */

			for ( p_hbin=file->block_list; p_hbin!=NULL; p_hbin=p_hbin->next ) {
				if ( p_hbin->file_off == hbin->file_off ) {
					cached = True;	
					break;
				}
			}

			block_off = hbin->file_off + hbin->block_size;

			if ( cached ) {
				prs_mem_free( &hbin->ps );
				hbin = NULL;
				continue;
			}
		}
	/* if (cached block or (new block and not enough free space)) then continue looping */
	} while ( cached || (hbin && (hbin->free_size < size)) );
	
	/* no free space; allocate a new one */

	if ( !hbin ) {
		uint32 alloc_size;

		/* allocate in multiples of REGF_ALLOC_BLOCK; make sure (size + hbin_header) fits */

		alloc_size = (((size+HBIN_HEADER_REC_SIZE) / REGF_ALLOC_BLOCK ) + 1 ) * REGF_ALLOC_BLOCK;

		if ( !(hbin = regf_hbin_allocate( file, alloc_size )) ) {
			DEBUG(0,("find_free_space: regf_hbin_allocate() failed!\n"));
			return NULL;
		}
		DLIST_ADD( file->block_list, hbin );
	}

done:
	/* set the offset to be ready to write */

	if ( !prs_set_offset( &hbin->ps, hbin->free_off-sizeof(uint32) ) )
		return NULL;

	/* write the record size as a placeholder for now, it should be
	   probably updated by the caller once it all of the data necessary 
	   for the record */

	if ( !prs_uint32("allocated_size", &hbin->ps, 0, &size) )
		return False;

	update_free_space( hbin, size );
	
	return hbin;
}

/*******************************************************************
*******************************************************************/

static uint32 sk_record_data_size( SEC_DESC * sd )
{
	uint32 size, size_mod8;

	size_mod8 = 0;

	/* the record size is sizeof(hdr) + name + static members + data_size_field */

	size = sizeof(uint32)*5 + sec_desc_size( sd ) + sizeof(uint32);

	/* multiple of 8 */
	size_mod8 = size & 0xfffffff8;
	if ( size_mod8 < size )
		size_mod8 += 8;

	return size_mod8;
}

/*******************************************************************
*******************************************************************/

static uint32 vk_record_data_size( REGF_VK_REC *vk )
{
	uint32 size, size_mod8;

	size_mod8 = 0;

	/* the record size is sizeof(hdr) + name + static members + data_size_field */

	size = REC_HDR_SIZE + (sizeof(uint16)*3) + (sizeof(uint32)*3) + sizeof(uint32);

	if ( vk->valuename )
		size += strlen(vk->valuename);

	/* multiple of 8 */
	size_mod8 = size & 0xfffffff8;
	if ( size_mod8 < size )
		size_mod8 += 8;

	return size_mod8;
}

/*******************************************************************
*******************************************************************/

static uint32 lf_record_data_size( uint32 num_keys )
{
	uint32 size, size_mod8;

	size_mod8 = 0;

	/* the record size is sizeof(hdr) + num_keys + sizeof of hash_array + data_size_uint32 */

	size = REC_HDR_SIZE + sizeof(uint16) + (sizeof(REGF_HASH_REC) * num_keys) + sizeof(uint32);

	/* multiple of 8 */
	size_mod8 = size & 0xfffffff8;
	if ( size_mod8 < size )
		size_mod8 += 8;

	return size_mod8;
}

/*******************************************************************
*******************************************************************/

static uint32 nk_record_data_size( REGF_NK_REC *nk )
{
	uint32 size, size_mod8;

	size_mod8 = 0;

	/* the record size is static + length_of_keyname + length_of_classname + data_size_uint32 */

	size = 0x4c + strlen(nk->keyname) + sizeof(uint32);

	if ( nk->classname )
		size += strlen( nk->classname );

	/* multiple of 8 */
	size_mod8 = size & 0xfffffff8;
	if ( size_mod8 < size )
		size_mod8 += 8;

	return size_mod8;
}

/*******************************************************************
*******************************************************************/

static BOOL create_vk_record( REGF_FILE *file, REGF_VK_REC *vk, REGISTRY_VALUE *value )
{
	char *name = regval_name(value);
	REGF_HBIN *data_hbin;

	ZERO_STRUCTP( vk );

	memcpy( vk->header, "vk", REC_HDR_SIZE );

	if ( name ) {
		vk->valuename = talloc_strdup( file->mem_ctx, regval_name(value) );
		vk->flag = VK_FLAG_NAME_PRESENT;
	}

	vk->data_size = regval_size( value );
	vk->type      = regval_type( value );

	if ( vk->data_size > sizeof(uint32) ) {
		uint32 data_size = ( (vk->data_size+sizeof(uint32)) & 0xfffffff8 ) + 8;

		vk->data = TALLOC_MEMDUP( file->mem_ctx, regval_data_p(value), vk->data_size );
		if (vk->data == NULL) {
			return False;
		}

		/* go ahead and store the offset....we'll pick this hbin block back up when 
		   we stream the data */

		if ((data_hbin = find_free_space(file, data_size )) == NULL) {
			return False;
		}
		vk->data_off = prs_offset( &data_hbin->ps ) + data_hbin->first_hbin_off - HBIN_HDR_SIZE;
	}
	else {
		/* make sure we don't try to copy from a NULL value pointer */

		if ( vk->data_size != 0 ) 
			memcpy( &vk->data_off, regval_data_p(value), sizeof(uint32) );
		vk->data_size |= VK_DATA_IN_OFFSET;		
	}

	return True;
}

/*******************************************************************
*******************************************************************/

static int hashrec_cmp( REGF_HASH_REC *h1, REGF_HASH_REC *h2 )
{
	return StrCaseCmp( h1->fullname, h2->fullname );
}

/*******************************************************************
*******************************************************************/

 REGF_NK_REC* regfio_write_key( REGF_FILE *file, const char *name, 
                               REGVAL_CTR *values, REGSUBKEY_CTR *subkeys, 
                               SEC_DESC *sec_desc, REGF_NK_REC *parent )
{
	REGF_NK_REC *nk;
	REGF_HBIN *vlist_hbin = NULL;
	uint32 size;

	if ( !(nk = TALLOC_ZERO_P( file->mem_ctx, REGF_NK_REC )) )
		return NULL;

	memcpy( nk->header, "nk", REC_HDR_SIZE );

	if ( !parent )
		nk->key_type = NK_TYPE_ROOTKEY;
	else
		nk->key_type = NK_TYPE_NORMALKEY;

	/* store the parent offset (or -1 if a the root key */

	nk->parent_off = parent ? (parent->hbin_off + parent->hbin->file_off - REGF_BLOCKSIZE - HBIN_HDR_SIZE ) : REGF_OFFSET_NONE;

	/* no classname currently */

	nk->classname_off = REGF_OFFSET_NONE;
	nk->classname = NULL;
	nk->keyname = talloc_strdup( file->mem_ctx, name );

	/* current modification time */

	unix_to_nt_time( &nk->mtime, time(NULL) );

	/* allocate the record on disk */

	size = nk_record_data_size( nk );
	nk->rec_size = ( size - 1 ) ^ 0XFFFFFFFF;
	if ((nk->hbin = find_free_space( file, size )) == NULL) {
		return NULL;
	}
	nk->hbin_off = prs_offset( &nk->hbin->ps );

	/* Update the hash record in the parent */
	
	if ( parent ) {
		REGF_HASH_REC *hash = &parent->subkeys.hashes[parent->subkey_index];

		hash->nk_off = prs_offset( &nk->hbin->ps ) + nk->hbin->first_hbin_off - HBIN_HDR_SIZE;
		memcpy( hash->keycheck, name, sizeof(uint32) );
		hash->fullname = talloc_strdup( file->mem_ctx, name );
		parent->subkey_index++;

		/* sort the list by keyname */

		qsort( parent->subkeys.hashes, parent->subkey_index, sizeof(REGF_HASH_REC), QSORT_CAST hashrec_cmp );

		if ( !hbin_prs_lf_records( "lf_rec", parent->subkeys.hbin, 0, parent ) )
			return False;
	}

	/* write the security descriptor */

	nk->sk_off = REGF_OFFSET_NONE;
	if ( sec_desc ) {
		uint32 sk_size = sk_record_data_size( sec_desc );
		REGF_HBIN *sk_hbin;
		REGF_SK_REC *tmp = NULL;

		/* search for it in the existing list of sd's */

		if ( (nk->sec_desc = find_sk_record_by_sec_desc( file, sec_desc )) == NULL ) {
			/* not found so add it to the list */

			if (!(sk_hbin = find_free_space( file, sk_size ))) {
				return NULL;
			}

			if ( !(nk->sec_desc = TALLOC_ZERO_P( file->mem_ctx, REGF_SK_REC )) )
				return NULL;
	
			/* now we have to store the security descriptor in the list and 
			   update the offsets */

			memcpy( nk->sec_desc->header, "sk", REC_HDR_SIZE );
			nk->sec_desc->hbin      = sk_hbin;
			nk->sec_desc->hbin_off  = prs_offset( &sk_hbin->ps );
			nk->sec_desc->sk_off    = prs_offset( &sk_hbin->ps ) + sk_hbin->first_hbin_off - HBIN_HDR_SIZE;
			nk->sec_desc->rec_size  = (sk_size-1)  ^ 0xFFFFFFFF;

			nk->sec_desc->sec_desc  = sec_desc;
			nk->sec_desc->ref_count = 0;
			
			/* size value must be self-inclusive */
			nk->sec_desc->size      = sec_desc_size(sec_desc) + sizeof(uint32);

			DLIST_ADD_END( file->sec_desc_list, nk->sec_desc, tmp );

			/* update the offsets for us and the previous sd in the list.
			   if this is the first record, then just set the next and prev
			   offsets to ourself. */

			if ( nk->sec_desc->prev ) {
				REGF_SK_REC *prev = nk->sec_desc->prev;

				nk->sec_desc->prev_sk_off = prev->hbin_off + prev->hbin->first_hbin_off - HBIN_HDR_SIZE;
				prev->next_sk_off = nk->sec_desc->sk_off;

				/* the end must loop around to the front */
				nk->sec_desc->next_sk_off = file->sec_desc_list->sk_off;

				/* and first must loop around to the tail */
				file->sec_desc_list->prev_sk_off = nk->sec_desc->sk_off;
			} else {
				nk->sec_desc->prev_sk_off = nk->sec_desc->sk_off;
				nk->sec_desc->next_sk_off = nk->sec_desc->sk_off;
			}
		}

		/* bump the reference count +1 */

		nk->sk_off = nk->sec_desc->sk_off;
		nk->sec_desc->ref_count++;
	}

	/* write the subkeys */

	nk->subkeys_off = REGF_OFFSET_NONE;
	if ( (nk->num_subkeys = regsubkey_ctr_numkeys( subkeys )) != 0 ) {
		uint32 lf_size = lf_record_data_size( nk->num_subkeys );
		uint32 namelen;
		int i;
		
		if (!(nk->subkeys.hbin = find_free_space( file, lf_size ))) {
			return NULL;
		}
		nk->subkeys.hbin_off = prs_offset( &nk->subkeys.hbin->ps );
		nk->subkeys.rec_size = (lf_size-1) ^ 0xFFFFFFFF;
		nk->subkeys_off = prs_offset( &nk->subkeys.hbin->ps ) + nk->subkeys.hbin->first_hbin_off - HBIN_HDR_SIZE;

		memcpy( nk->subkeys.header, "lf", REC_HDR_SIZE );
		
		nk->subkeys.num_keys = nk->num_subkeys;
		if ( !(nk->subkeys.hashes = TALLOC_ZERO_ARRAY( file->mem_ctx, REGF_HASH_REC, nk->subkeys.num_keys )) )
			return NULL;
		nk->subkey_index = 0;

		/* update the max_bytes_subkey{name,classname} fields */
		for ( i=0; i<nk->num_subkeys; i++ ) {
			namelen = strlen( regsubkey_ctr_specific_key(subkeys, i) );
			if ( namelen*2 > nk->max_bytes_subkeyname )
				nk->max_bytes_subkeyname = namelen * 2;
		}
	}

	/* write the values */

	nk->values_off = REGF_OFFSET_NONE;
	if ( (nk->num_values = regval_ctr_numvals( values )) != 0 ) {
		uint32 vlist_size = ( ( nk->num_values * sizeof(uint32) ) & 0xfffffff8 ) + 8;
		int i;
		
		if (!(vlist_hbin = find_free_space( file, vlist_size ))) {
			return NULL;
		}
		nk->values_off = prs_offset( &vlist_hbin->ps ) + vlist_hbin->first_hbin_off - HBIN_HDR_SIZE;
	
		if ( !(nk->values = TALLOC_ARRAY( file->mem_ctx, REGF_VK_REC, nk->num_values )) )
			return NULL;

		/* create the vk records */

		for ( i=0; i<nk->num_values; i++ ) {
			uint32 vk_size, namelen, datalen;
			REGISTRY_VALUE *r;

			r = regval_ctr_specific_value( values, i );
			create_vk_record( file, &nk->values[i], r );
			vk_size = vk_record_data_size( &nk->values[i] );
			nk->values[i].hbin = find_free_space( file, vk_size );
			nk->values[i].hbin_off = prs_offset( &nk->values[i].hbin->ps );
			nk->values[i].rec_size = ( vk_size - 1 ) ^ 0xFFFFFFFF;
			nk->values[i].rec_off = prs_offset( &nk->values[i].hbin->ps ) 
				+ nk->values[i].hbin->first_hbin_off 
				- HBIN_HDR_SIZE;

			/* update the max bytes fields if necessary */

			namelen = strlen( regval_name(r) );
			if ( namelen*2 > nk->max_bytes_valuename )
				nk->max_bytes_valuename = namelen * 2;

			datalen = regval_size( r );
			if ( datalen > nk->max_bytes_value )
				nk->max_bytes_value = datalen;
		}
	}

	/* stream the records */	
	
	prs_set_offset( &nk->hbin->ps, nk->hbin_off );
	if ( !prs_nk_rec( "nk_rec", &nk->hbin->ps, 0, nk ) )
		return False;

	if ( nk->num_values ) {
		if ( !hbin_prs_vk_records( "vk_records", vlist_hbin, 0, nk, file ) )
			return False;
	}


	regfio_flush( file );

	return nk;
}

