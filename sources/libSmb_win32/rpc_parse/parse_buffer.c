/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 * 
 *  Copyright (C) Andrew Tridgell              1992-2000,
 *  Copyright (C) Luke Kenneth Casson Leighton 1996-2000,
 *  Copyright (C) Jean François Micouleau      1998-2000,
 *  Copyright (C) Gerald Carter                2000-2005,
 *  Copyright (C) Tim Potter		       2001-2002.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_PARSE

/**********************************************************************
 Initialize a new spoolss buff for use by a client rpc
**********************************************************************/
void rpcbuf_init(RPC_BUFFER *buffer, uint32 size, TALLOC_CTX *ctx)
{
	buffer->size = size;
	buffer->string_at_end = size;
	prs_init(&buffer->prs, size, ctx, MARSHALL);
	buffer->struct_start = prs_offset(&buffer->prs);
}

/*******************************************************************
 Read/write a RPC_BUFFER struct.
********************************************************************/  

BOOL prs_rpcbuffer(const char *desc, prs_struct *ps, int depth, RPC_BUFFER *buffer)
{
	prs_debug(ps, depth, desc, "prs_rpcbuffer");
	depth++;

	/* reading */
	if (UNMARSHALLING(ps)) {
		buffer->size=0;
		buffer->string_at_end=0;
		
		if (!prs_uint32("size", ps, depth, &buffer->size))
			return False;
					
		/*
		 * JRA. I'm not sure if the data in here is in big-endian format if
		 * the client is big-endian. Leave as default (little endian) for now.
		 */

		if (!prs_init(&buffer->prs, buffer->size, prs_get_mem_context(ps), UNMARSHALL))
			return False;

		if (!prs_append_some_prs_data(&buffer->prs, ps, prs_offset(ps), buffer->size))
			return False;

		if (!prs_set_offset(&buffer->prs, 0))
			return False;

		if (!prs_set_offset(ps, buffer->size+prs_offset(ps)))
			return False;

		buffer->string_at_end=buffer->size;
		
		return True;
	}
	else {
		BOOL ret = False;

		if (!prs_uint32("size", ps, depth, &buffer->size))
			goto out;

		if (!prs_append_some_prs_data(ps, &buffer->prs, 0, buffer->size))
			goto out;

		ret = True;
	out:

		/* We have finished with the data in buffer->prs - free it. */
		prs_mem_free(&buffer->prs);

		return ret;
	}
}

/*******************************************************************
 Read/write an RPC_BUFFER* struct.(allocate memory if unmarshalling)
********************************************************************/  

BOOL prs_rpcbuffer_p(const char *desc, prs_struct *ps, int depth, RPC_BUFFER **buffer)
{
	uint32 data_p;

	/* caputure the pointer value to stream */

	data_p = *buffer ? 0xf000baaa : 0;

	if ( !prs_uint32("ptr", ps, depth, &data_p ))
		return False;

	/* we're done if there is no data */

	if ( !data_p )
		return True;

	if ( UNMARSHALLING(ps) ) {
		if ( !(*buffer = PRS_ALLOC_MEM(ps, RPC_BUFFER, 1)) )
			return False;
	} else {
		/* Marshalling case. - coverity paranoia - should already be ok if data_p != 0 */
		if (!*buffer) {
			return True;
		}
	}

	return prs_rpcbuffer( desc, ps, depth, *buffer);
}

/****************************************************************************
 Allocate more memory for a RPC_BUFFER.
****************************************************************************/

BOOL rpcbuf_alloc_size(RPC_BUFFER *buffer, uint32 buffer_size)
{
	prs_struct *ps;
	uint32 extra_space;
	uint32 old_offset;
	
	/* if we don't need anything. don't do anything */
	
	if ( buffer_size == 0x0 )
		return True;

	if (!buffer) {
		return False;
	}

	ps= &buffer->prs;

	/* damn, I'm doing the reverse operation of prs_grow() :) */
	if (buffer_size < prs_data_size(ps))
		extra_space=0;
	else	
		extra_space = buffer_size - prs_data_size(ps);

	/*
	 * save the offset and move to the end of the buffer
	 * prs_grow() checks the extra_space against the offset
	 */
	old_offset=prs_offset(ps);	
	prs_set_offset(ps, prs_data_size(ps));
	
	if (!prs_grow(ps, extra_space))
		return False;

	prs_set_offset(ps, old_offset);

	buffer->string_at_end=prs_data_size(ps);

	return True;
}

/*******************************************************************
 move a BUFFER from the query to the reply.
 As the data pointers in RPC_BUFFER are malloc'ed, not talloc'ed,
 this is ok. This is an OPTIMIZATION and is not strictly neccessary.
 Clears the memory to zero also.
********************************************************************/  

void rpcbuf_move(RPC_BUFFER *src, RPC_BUFFER **dest)
{
	if ( !src ) {
		*dest = NULL;
		return;
	}

	prs_switch_type( &src->prs, MARSHALL );

	if ( !prs_set_offset(&src->prs, 0) )
		return;

	prs_force_dynamic( &src->prs );
	prs_mem_clear( &src->prs );

	*dest = src;
}

/*******************************************************************
 Get the size of a BUFFER struct.
********************************************************************/  

uint32 rpcbuf_get_size(RPC_BUFFER *buffer)
{
	return (buffer->size);
}


/*******************************************************************
 * write a UNICODE string and its relative pointer.
 * used by all the RPC structs passing a buffer
 *
 * As I'm a nice guy, I'm forcing myself to explain this code.
 * MS did a good job in the overall spoolss code except in some
 * functions where they are passing the API buffer directly in the
 * RPC request/reply. That's to maintain compatiility at the API level.
 * They could have done it the good way the first time.
 *
 * So what happen is: the strings are written at the buffer's end, 
 * in the reverse order of the original structure. Some pointers to
 * the strings are also in the buffer. Those are relative to the
 * buffer's start.
 *
 * If you don't understand or want to change that function,
 * first get in touch with me: jfm@samba.org
 *
 ********************************************************************/

BOOL smb_io_relstr(const char *desc, RPC_BUFFER *buffer, int depth, UNISTR *string)
{
	prs_struct *ps=&buffer->prs;
	
	if (MARSHALLING(ps)) {
		uint32 struct_offset = prs_offset(ps);
		uint32 relative_offset;
		
		buffer->string_at_end -= (size_of_relative_string(string) - 4);
		if(!prs_set_offset(ps, buffer->string_at_end))
			return False;
#if 0	/* JERRY */
		/*
		 * Win2k does not align strings in a buffer
		 * Tested against WinNT 4.0 SP 6a & 2k SP2  --jerry
		 */
		if (!prs_align(ps))
			return False;
#endif
		buffer->string_at_end = prs_offset(ps);
		
		/* write the string */
		if (!smb_io_unistr(desc, string, ps, depth))
			return False;

		if(!prs_set_offset(ps, struct_offset))
			return False;
		
		relative_offset=buffer->string_at_end - buffer->struct_start;
		/* write its offset */
		if (!prs_uint32("offset", ps, depth, &relative_offset))
			return False;
	}
	else {
		uint32 old_offset;
		
		/* read the offset */
		if (!prs_uint32("offset", ps, depth, &(buffer->string_at_end)))
			return False;

		if (buffer->string_at_end == 0)
			return True;

		old_offset = prs_offset(ps);
		if(!prs_set_offset(ps, buffer->string_at_end+buffer->struct_start))
			return False;

		/* read the string */
		if (!smb_io_unistr(desc, string, ps, depth))
			return False;

		if(!prs_set_offset(ps, old_offset))
			return False;
	}
	return True;
}

/*******************************************************************
 * write a array of UNICODE strings and its relative pointer.
 * used by 2 RPC structs
 ********************************************************************/

BOOL smb_io_relarraystr(const char *desc, RPC_BUFFER *buffer, int depth, uint16 **string)
{
	UNISTR chaine;
	
	prs_struct *ps=&buffer->prs;
	
	if (MARSHALLING(ps)) {
		uint32 struct_offset = prs_offset(ps);
		uint32 relative_offset;
		uint16 *p;
		uint16 *q;
		uint16 zero=0;
		p=*string;
		q=*string;

		/* first write the last 0 */
		buffer->string_at_end -= 2;
		if(!prs_set_offset(ps, buffer->string_at_end))
			return False;

		if(!prs_uint16("leading zero", ps, depth, &zero))
			return False;

		while (p && (*p!=0)) {	
			while (*q!=0)
				q++;

			/* Yes this should be malloc not talloc. Don't change. */

			chaine.buffer = SMB_MALLOC((q-p+1)*sizeof(uint16));
			if (chaine.buffer == NULL)
				return False;

			memcpy(chaine.buffer, p, (q-p+1)*sizeof(uint16));

			buffer->string_at_end -= (q-p+1)*sizeof(uint16);

			if(!prs_set_offset(ps, buffer->string_at_end)) {
				SAFE_FREE(chaine.buffer);
				return False;
			}

			/* write the string */
			if (!smb_io_unistr(desc, &chaine, ps, depth)) {
				SAFE_FREE(chaine.buffer);
				return False;
			}
			q++;
			p=q;

			SAFE_FREE(chaine.buffer);
		}
		
		if(!prs_set_offset(ps, struct_offset))
			return False;
		
		relative_offset=buffer->string_at_end - buffer->struct_start;
		/* write its offset */
		if (!prs_uint32("offset", ps, depth, &relative_offset))
			return False;

	} else {

		/* UNMARSHALLING */

		uint32 old_offset;
		uint16 *chaine2=NULL;
		int l_chaine=0;
		int l_chaine2=0;
		size_t realloc_size = 0;

		*string=NULL;
				
		/* read the offset */
		if (!prs_uint32("offset", ps, depth, &buffer->string_at_end))
			return False;

		old_offset = prs_offset(ps);
		if(!prs_set_offset(ps, buffer->string_at_end + buffer->struct_start))
			return False;
	
		do {
			if (!smb_io_unistr(desc, &chaine, ps, depth))
				return False;
			
			l_chaine=str_len_uni(&chaine);
			
			/* we're going to add two more bytes here in case this
			   is the last string in the array and we need to add 
			   an extra NULL for termination */
			if (l_chaine > 0) {
				realloc_size = (l_chaine2+l_chaine+2)*sizeof(uint16);

				/* Yes this should be realloc - it's freed below. JRA */

				if((chaine2=(uint16 *)SMB_REALLOC(chaine2, realloc_size)) == NULL) {
					return False;
				}
				memcpy(chaine2+l_chaine2, chaine.buffer, (l_chaine+1)*sizeof(uint16));
				l_chaine2+=l_chaine+1;
			}
		
		} while(l_chaine!=0);
		
		/* the end should be bould NULL terminated so add 
		   the second one here */
		if (chaine2)
		{
			chaine2[l_chaine2] = '\0';
			*string=(uint16 *)TALLOC_MEMDUP(prs_get_mem_context(ps),chaine2,realloc_size);
			SAFE_FREE(chaine2);
		}

		if(!prs_set_offset(ps, old_offset))
			return False;
	}
	return True;
}

/*******************************************************************
 Parse a DEVMODE structure and its relative pointer.
********************************************************************/

BOOL smb_io_relsecdesc(const char *desc, RPC_BUFFER *buffer, int depth, SEC_DESC **secdesc)
{
	prs_struct *ps= &buffer->prs;

	prs_debug(ps, depth, desc, "smb_io_relsecdesc");
	depth++;

	if (MARSHALLING(ps)) {
		uint32 struct_offset = prs_offset(ps);
		uint32 relative_offset;

		if (! *secdesc) {
			relative_offset = 0;
			if (!prs_uint32("offset", ps, depth, &relative_offset))
				return False;
			return True;
		}
		
		if (*secdesc != NULL) {
			buffer->string_at_end -= sec_desc_size(*secdesc);

			if(!prs_set_offset(ps, buffer->string_at_end))
				return False;
			/* write the secdesc */
			if (!sec_io_desc(desc, secdesc, ps, depth))
				return False;

			if(!prs_set_offset(ps, struct_offset))
				return False;
		}

		relative_offset=buffer->string_at_end - buffer->struct_start;
		/* write its offset */

		if (!prs_uint32("offset", ps, depth, &relative_offset))
			return False;
	} else {
		uint32 old_offset;
		
		/* read the offset */
		if (!prs_uint32("offset", ps, depth, &buffer->string_at_end))
			return False;

		old_offset = prs_offset(ps);
		if(!prs_set_offset(ps, buffer->string_at_end + buffer->struct_start))
			return False;

		/* read the sd */
		if (!sec_io_desc(desc, secdesc, ps, depth))
			return False;

		if(!prs_set_offset(ps, old_offset))
			return False;
	}
	return True;
}



/*******************************************************************
 * return the length of a UNICODE string in number of char, includes:
 * - the leading zero
 * - the relative pointer size
 ********************************************************************/

uint32 size_of_relative_string(UNISTR *string)
{
	uint32 size=0;
	
	size=str_len_uni(string);	/* the string length       */
	size=size+1;			/* add the trailing zero   */
	size=size*2;			/* convert in char         */
	size=size+4;			/* add the size of the ptr */	

#if 0	/* JERRY */
	/* 
	 * Do not include alignment as Win2k does not align relative
	 * strings within a buffer   --jerry 
	 */
	/* Ensure size is 4 byte multiple (prs_align is being called...). */
	/* size += ((4 - (size & 3)) & 3); */
#endif 

	return size;
}

