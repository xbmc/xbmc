/* cbuf2.c
 * circular buffer lib - version 2
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "relaylib.h"
#include "cbuf2.h"
#include "debug.h"
#include "ripogg.h"

/*****************************************************************************
 * Global variables
 *****************************************************************************/
/* The circular buffer */
CBUF2 g_cbuf2;

/*****************************************************************************
 * Notes:
 *
 *   The terminology "count" should refer to a length, starting from 
 *   the first filled byte.
 *
 *   The terminology "offset" == "count"
 *
 *   The terminology "pos" should refer to the absolute array index, 
 *   starting from the beginning of the allocated buffer.
 *
 *   The terminology "idx" == "pos"
 *
 *   The read & write indices always point to the next byte to be read 
 *   or written.  When the buffer is full or empty, they are equal.
 *   Use the item_count to distinguish these cases.
 *****************************************************************************/
/*****************************************************************************
 * Private functions
 *****************************************************************************/
static u_long cbuf2_idx_to_chunk (CBUF2 *cbuf2, u_long idx);
static u_long cbuf2_add (CBUF2 *cbuf2, u_long pos, u_long len);
static void cbuf2_get_used_fragments (CBUF2 *cbuf2, u_long* frag1, 
				      u_long* frag2);
static void cbuf2_advance_metadata_list (CBUF2* cbuf2);
static error_code cbuf2_extract_relay_mp3 (CBUF2 *cbuf2, char *data, 
					   u_long *pos, u_long *len, 
					   int icy_metadata);
static error_code cbuf2_extract_relay_ogg (CBUF2 *cbuf2, RELAY_LIST* ptr);
static u_long cbuf2_offset (CBUF2 *cbuf2, u_long pos);
static u_long cbuf2_offset_2 (CBUF2 *cbuf2, u_long pos);
#if defined (commentout)
static u_long cbuf2_subtract (CBUF2 *cbuf2, u_long pos1, u_long pos2);
#endif

/*****************************************************************************
 * Function definitions
 *****************************************************************************/
error_code
cbuf2_init (CBUF2 *cbuf2, int content_type, int have_relay, 
	    unsigned long chunk_size, unsigned long num_chunks)
{
    if (chunk_size == 0 || num_chunks == 0) {
        return SR_ERROR_INVALID_PARAM;
    }
    cbuf2->have_relay = have_relay;
    cbuf2->content_type = content_type;
    cbuf2->chunk_size = chunk_size;
    cbuf2->num_chunks = num_chunks;
    cbuf2->size = chunk_size * num_chunks;
    cbuf2->buf = (char*) malloc (cbuf2->size);
    cbuf2->base_idx = 0;
    cbuf2->item_count = 0;
    cbuf2->next_song = 0;        /* MP3 only */
    cbuf2->song_page = 0;        /* OGG only */
    cbuf2->song_page_done = 0;   /* OGG only */

    INIT_LIST_HEAD (&cbuf2->metadata_list);
    INIT_LIST_HEAD (&cbuf2->ogg_page_list);

    cbuf2->cbuf_sem = threadlib_create_sem();
    threadlib_signal_sem (&cbuf2->cbuf_sem);

    return SR_SUCCESS;
}

void
cbuf2_destroy (CBUF2 *cbuf2)
{
    if (cbuf2->buf) {
	free (cbuf2->buf);
	cbuf2->buf = NULL;
	/* GCS FIX: Delete metadata queue */
	threadlib_destroy_sem(&cbuf2->cbuf_sem);
    }
}

static error_code
cbuf2_insert (CBUF2 *cbuf2, const char *data, u_long count)
{
    u_long ft;
    u_long write_idx;

    if (cbuf2_get_free (cbuf2) < count) {
	return SR_ERROR_BUFFER_FULL;
    }
    
    write_idx = cbuf2_write_index (cbuf2);
    ft = cbuf2_get_free_tail (cbuf2);
    if (ft >= count) {
	memcpy (&cbuf2->buf[write_idx], data, count);
    } else {
	memcpy (&cbuf2->buf[write_idx], data, ft);
	memcpy (&cbuf2->buf[0], &data[ft], count-ft);
    }
    cbuf2->item_count += count;
    return SR_SUCCESS;
}

static void
cbuf2_advance_metadata_list (CBUF2* cbuf2)
{
    u_long frag1, frag2;
    u_long metadata_pos;
    u_long chunk_no;
    LIST *p;
    METADATA_LIST *tmp;

    cbuf2_get_used_fragments (cbuf2, &frag1, &frag2);
    chunk_no = cbuf2_idx_to_chunk (cbuf2, cbuf2->base_idx);

    /* Loop through all metadata, from tail to head */
    list_for_each_prev (p, &(cbuf2->metadata_list)) {
    
	/* Get position of metadata */
	tmp = list_entry(p, METADATA_LIST, m_list);
	metadata_pos = (tmp->m_chunk + 1) * cbuf2->chunk_size - 1;
	debug_printf ("CE3: %d\n", metadata_pos);

	if (metadata_pos >= cbuf2->base_idx 
	    && metadata_pos < cbuf2->base_idx + frag1) {
	    /* Nothing to do */
	} else if (frag2 > 0 && metadata_pos <= frag2) {
	    /* Nothing to do */
	} else {
	    /* Push metadata forward to most recent read pointer */
	    tmp->m_chunk = chunk_no;
	}
	/* If metadata is at head, evict all previous metadata from the list */
	debug_printf ("CE4: %d %d\n", tmp->m_chunk, chunk_no);
	if (tmp->m_chunk == chunk_no) {
	    while (p->prev != &cbuf2->metadata_list) {
		LIST* pp = p->prev;
		list_del (p->prev);
		free (list_entry (pp,METADATA_LIST,m_list));
	    }
	}
    }
    debug_printf ("CBUF_EXTRACT\n");
    cbuf2_debug_lists (cbuf2);
}

static void
cbuf2_advance_relay_list (CBUF2* cbuf2, u_long count)
{
    RELAY_LIST *prev, *ptr, *next;

    ptr = g_relay_list;
    if (ptr == NULL) {
	return;
    }
    prev = NULL;
    while (ptr != NULL) {
	next = ptr->m_next;
	if (!ptr->m_is_new) {
	    if (ptr->m_cbuf_pos >= count) {
		u_long old_pos = ptr->m_cbuf_pos;
		ptr->m_cbuf_pos -= count;
		debug_printf ("Updated relay pointer: %d -> %d\n",
			      old_pos, ptr->m_cbuf_pos);
		prev = ptr;
	    } else {
		debug_printf ("Relay: Client %d couldn't keep up with cbuf\n", 
			      ptr->m_sock);
		relaylib_disconnect (prev, ptr);
	    }
	}
	ptr = next;
    }
}

void
cbuf2_set_next_song (CBUF2 *cbuf2, u_long pos)
{
    cbuf2->next_song = pos;
}

/* Advance the cbuffer while respecting ogg page boundaries */
error_code
cbuf2_advance_ogg (CBUF2 *cbuf2, int requested_free_size)
{
    u_long count;
    LIST *p, *n;
    OGG_PAGE_LIST *tmp;

    if (cbuf2->item_count + requested_free_size <= cbuf2->size) {
	return SR_SUCCESS;
    }

    if (cbuf2->have_relay) {
	debug_printf ("Waiting for g_relay_list_sem\n");
	threadlib_waitfor_sem (&g_relay_list_sem);
    }
    debug_printf ("Waiting for cbuf2->cbuf_sem\n");
    threadlib_waitfor_sem (&cbuf2->cbuf_sem);

    /* count is the amount we're going to free up */
    count = requested_free_size - (cbuf2->size - cbuf2->item_count);

    /* Loop through ogg pages, from head to tail. Eject pages that 
       that map into the portion of the buffer that we are freeing. */
    p = cbuf2->ogg_page_list.next;
    list_for_each_safe (p, n, &(cbuf2->ogg_page_list)) {
	u_long pos;
	tmp = list_entry(p, OGG_PAGE_LIST, m_list);
	pos = cbuf2_offset (cbuf2, tmp->m_page_start);
	if (pos > count) {
	    break;
	}
	list_del (p);
	if (tmp->m_page_flags & OGG_PAGE_EOS) {
	    free (tmp->m_header_buf_ptr);
	}
	free (tmp);
	if (cbuf2->item_count + requested_free_size <= cbuf2->size) {
	    break;
	}
    }
    cbuf2->base_idx = cbuf2_add (cbuf2, cbuf2->base_idx, count);
    cbuf2->item_count -= count;

    if (cbuf2->have_relay) {
	cbuf2_advance_relay_list (cbuf2, count);
    }
    threadlib_signal_sem (&cbuf2->cbuf_sem);
    if (cbuf2->have_relay) {
	threadlib_signal_sem (&g_relay_list_sem);
    }

    if (cbuf2->item_count + requested_free_size <= cbuf2->size) {
	return SR_SUCCESS;
    } else {
	return SR_ERROR_BUFFER_EMPTY;
    }
}

/* On extract, we need to update all the relay pointers. */
/* GCS Now it always extracts a chunk */
error_code
cbuf2_extract (CBUF2 *cbuf2, char *data, u_long count, u_long* curr_song)
{
    u_long frag1, frag2;

    /* Don't need to lock this, only the ripping thread changes it. */
    if (cbuf2->item_count < count) {
	return SR_ERROR_BUFFER_EMPTY;
    }

    if (cbuf2->have_relay) {
	debug_printf ("Waiting for g_relay_list_sem\n");
	threadlib_waitfor_sem (&g_relay_list_sem);
    }
    debug_printf ("Waiting for cbuf2->cbuf_sem\n");
    threadlib_waitfor_sem (&cbuf2->cbuf_sem);
    cbuf2_get_used_fragments (cbuf2, &frag1, &frag2);
    if (frag1 >= count) {
	memcpy (data, &cbuf2->buf[cbuf2->base_idx], count);
    } else {
	debug_printf ("This should never happen\n");
	memcpy (data, &cbuf2->buf[cbuf2->base_idx], frag1);
	memcpy (&data[frag1], &cbuf2->buf[0], count-frag1);
    }
    cbuf2->base_idx = cbuf2_add (cbuf2, cbuf2->base_idx, count);
    cbuf2->item_count -= count;
    debug_printf ("CE2: %d\n", cbuf2->base_idx);

    *curr_song = cbuf2->next_song;
    if (cbuf2->next_song < count) {
	cbuf2->next_song = 0;
    } else {
	cbuf2->next_song -= count;
    }

    cbuf2_advance_metadata_list (cbuf2);

    if (cbuf2->have_relay) {
	cbuf2_advance_relay_list (cbuf2, count);
    }
    threadlib_signal_sem (&cbuf2->cbuf_sem);
    if (cbuf2->have_relay) {
	threadlib_signal_sem (&g_relay_list_sem);
    }
    return SR_SUCCESS;
}

error_code
cbuf2_fastforward (CBUF2 *cbuf2, u_long count)
{
    threadlib_waitfor_sem (&cbuf2->cbuf_sem);
    if (cbuf2->item_count < count) {
	threadlib_signal_sem (&cbuf2->cbuf_sem);
	return SR_ERROR_BUFFER_EMPTY;
    }
    cbuf2->base_idx = cbuf2_add (cbuf2, cbuf2->base_idx, count);
    cbuf2->item_count -= count;

    threadlib_signal_sem (&cbuf2->cbuf_sem);
    return SR_SUCCESS;
}

error_code
cbuf2_peek (CBUF2 *cbuf2, char *data, u_long count)
{
    u_long frag1, frag2;

    if (cbuf2->item_count < count) {
	return SR_ERROR_BUFFER_EMPTY;
    }

    threadlib_waitfor_sem (&cbuf2->cbuf_sem);
    cbuf2_get_used_fragments (cbuf2, &frag1, &frag2);
    if (frag1 >= count) {
	memcpy (data, &cbuf2->buf[cbuf2->base_idx], count);
    } else {
	memcpy (data, &cbuf2->buf[cbuf2->base_idx], frag1);
	memcpy (&data[frag1], &cbuf2->buf[0], count-frag1);
    }
    threadlib_signal_sem (&cbuf2->cbuf_sem);
    return SR_SUCCESS;
}

error_code
cbuf2_insert_chunk (CBUF2 *cbuf2, const char *data, u_long count,
		    int content_type, TRACK_INFO* ti)
{
    LIST this_page_list;
    int chunk_no;
    error_code rc;

    /* Identify OGG pages using rip_ogg_process_chunk(). 
       We can do this before locking the CBUF to improve 
       concurrency with relay threads. */
    if (content_type == CONTENT_TYPE_OGG) {
	OGG_PAGE_LIST* tmp;
	LIST *p;
	unsigned long page_loc;

	rip_ogg_process_chunk (&this_page_list, data, count, ti);

	if (list_empty(&cbuf2->ogg_page_list)) {
	    page_loc = 0;
	} else {
	    p = cbuf2->ogg_page_list.prev;
	    tmp = list_entry(p, OGG_PAGE_LIST, m_list);
	    page_loc = cbuf2_add (cbuf2, tmp->m_page_start, tmp->m_page_len);
	}
	
	// p = &this_page_list.next;
	list_for_each (p, &this_page_list) {
	    tmp = list_entry(p, OGG_PAGE_LIST, m_list);
	    tmp->m_page_start = page_loc;
	    page_loc = cbuf2_add (cbuf2, page_loc, tmp->m_page_len);
	}
    }

    /* Insert data */
    threadlib_waitfor_sem (&cbuf2->cbuf_sem);
    chunk_no = cbuf2_idx_to_chunk (cbuf2, cbuf2_write_index(cbuf2));
    rc = cbuf2_insert (cbuf2, data, count);
    if (rc != SR_SUCCESS) {
	threadlib_signal_sem (&cbuf2->cbuf_sem);
	return rc;
    }

    if (content_type == CONTENT_TYPE_OGG) {
	list_splice (&this_page_list, cbuf2->ogg_page_list.prev);
    } else if (ti && ti->have_track_info) {
	/* Insert metadata data */
#if defined (commentout)
	int num_bytes;
	unsigned char num_16_bytes;
#endif
	METADATA_LIST* ml;
	ml = (METADATA_LIST*) malloc (sizeof(METADATA_LIST));
	/* GCS FIX: Check malloc error */
	ml->m_chunk = chunk_no;
#if defined (commentout)
	num_bytes = snprintf (&ml->m_composed_metadata[1], MAX_METADATA_LEN,
			      "StreamTitle='%s - %s';", 
			      ti->artist, ti->title);
	ml->m_composed_metadata[MAX_METADATA_LEN] = 0;  // note, not LEN-1
	num_16_bytes = (num_bytes + 15) / 16;
	ml->m_composed_metadata[0] = num_16_bytes;
#endif
	memcpy (ml->m_composed_metadata, ti->composed_metadata, 
		MAX_METADATA_LEN+1);
	list_add_tail (&(ml->m_list), &(cbuf2->metadata_list));
    }
    debug_printf ("CBUF_INSERT\n");
    cbuf2_debug_lists (cbuf2);

    threadlib_signal_sem (&cbuf2->cbuf_sem);
    return SR_SUCCESS;
}

/* Relay always gets one chunk.  Return 1 if we got a chunk, 
   return 0 if the buffer is empty. 
*/
error_code 
cbuf2_extract_relay (CBUF2 *cbuf2, RELAY_LIST* ptr)
{
    if (cbuf2->content_type == CONTENT_TYPE_OGG) {
	return cbuf2_extract_relay_ogg (cbuf2, ptr);
    } else {
	return cbuf2_extract_relay_mp3 (cbuf2, ptr->m_buffer, 
					&ptr->m_cbuf_pos, &ptr->m_left_to_send,
					ptr->m_icy_metadata);
    }
}

static error_code 
cbuf2_extract_relay_ogg (CBUF2 *cbuf2, RELAY_LIST* ptr)
{
    if (ptr->m_header_buf_ptr) {
	u_long remaining = ptr->m_header_buf_len - ptr->m_header_buf_off;
	if (remaining > ptr->m_buffer_size) {
	    memcpy (ptr->m_buffer, 
		    &ptr->m_header_buf_ptr[ptr->m_header_buf_off], 
		    ptr->m_buffer_size);
	    ptr->m_header_buf_off += ptr->m_buffer_size;
	    ptr->m_left_to_send = ptr->m_buffer_size;
	} else {
	    memcpy (ptr->m_buffer, 
		    &ptr->m_header_buf_ptr[ptr->m_header_buf_off], 
		    remaining);
	    ptr->m_header_buf_ptr = 0;
	    ptr->m_header_buf_off = 0;
	    ptr->m_header_buf_len = 0;
	    ptr->m_left_to_send = remaining;
	}
    } else {
	u_long frag1, frag2;
	u_long relay_idx, remaining;

	threadlib_waitfor_sem (&cbuf2->cbuf_sem);
	relay_idx = cbuf2_add (cbuf2, cbuf2->base_idx, ptr->m_cbuf_pos);
	cbuf2_get_used_fragments (cbuf2, &frag1, &frag2);
	if (ptr->m_cbuf_pos < frag1) {
	    remaining = frag1 - ptr->m_cbuf_pos;
	} else {
	    remaining = cbuf2->item_count - ptr->m_cbuf_pos;
	}
	debug_printf ("%p p=%d/t=%d (f1=%d,f2=%d) [r=%d/bs=%d]\n", 
		      ptr,
		      ptr->m_cbuf_pos, cbuf2->item_count,
		      frag1, frag2,
		      remaining, ptr->m_buffer_size);
	if (remaining == 0) {
	    threadlib_signal_sem (&cbuf2->cbuf_sem);
	    return SR_ERROR_BUFFER_EMPTY;
	}
	if (remaining > ptr->m_buffer_size) {
	    memcpy (ptr->m_buffer, &cbuf2->buf[relay_idx], ptr->m_buffer_size);
	    ptr->m_left_to_send = ptr->m_buffer_size;
	    ptr->m_cbuf_pos += ptr->m_buffer_size;
	} else {
	    memcpy (ptr->m_buffer, &cbuf2->buf[relay_idx], remaining);
	    ptr->m_left_to_send = remaining;
	    ptr->m_cbuf_pos += remaining;
	}
	threadlib_signal_sem (&cbuf2->cbuf_sem);
    }
    return SR_SUCCESS;
}

static error_code 
cbuf2_extract_relay_mp3 (CBUF2 *cbuf2, char *data, u_long *pos, u_long *len,
			 int icy_metadata)
{
    u_long frag1, frag2;
    u_long relay_chunk_no;
    u_long relay_idx;
    u_long old_pos;
    *len = 0;

    /* Check if there is data to send to the relay */
    threadlib_waitfor_sem (&cbuf2->cbuf_sem);
    cbuf2_get_used_fragments (cbuf2, &frag1, &frag2);
    if (*pos + cbuf2->chunk_size > cbuf2->item_count) {
	threadlib_signal_sem (&cbuf2->cbuf_sem);
	return SR_ERROR_BUFFER_EMPTY;
    }

    /* Otherwise, there is enough data */
    /* The *pos is the offset from the filled part of the buffer */
    old_pos = *pos;
    relay_idx = cbuf2_add (cbuf2, cbuf2->base_idx, *pos);
    relay_chunk_no = cbuf2_idx_to_chunk (cbuf2, *pos);
    memcpy (data, &cbuf2->buf[relay_idx], cbuf2->chunk_size);
    *pos += cbuf2->chunk_size;
    *len = cbuf2->chunk_size;
    debug_printf ("Updated relay pointer: %d -> %d (%d %d %d)\n",
		  old_pos, *pos, cbuf2->base_idx,
		  relay_idx, cbuf2_write_index(cbuf2));
    debug_printf ("Partial request fulfilled: %d bytes (%d)\n", *len, 
		  icy_metadata);
    if (icy_metadata) {
	int have_metadata = 0;
	LIST *p = cbuf2->metadata_list.next;
	METADATA_LIST *tmp;
	list_for_each (p, &(cbuf2->metadata_list)) {
	    tmp = list_entry(p, METADATA_LIST, m_list);
	    debug_printf ("Comparing metadata: meta=%d relay=%d\n",
			  tmp->m_chunk, relay_chunk_no);
	    if (tmp->m_chunk == relay_chunk_no) {
		debug_printf ("Got metadata!\n");
		have_metadata = 1;
		break;
	    }
	}
	if (have_metadata) {
	    int metadata_len = tmp->m_composed_metadata[0] * 16 + 1;
	    memcpy (&data[cbuf2->chunk_size],tmp->m_composed_metadata,
		    metadata_len);
	    *len += metadata_len;
	} else {
	    data[cbuf2->chunk_size] = '\0';
	    (*len) ++;
	}
    }
    debug_printf ("Final request fulfilled: %d bytes\n", *len);
    threadlib_signal_sem (&cbuf2->cbuf_sem);
    return SR_SUCCESS;
}

void
cbuf2_debug_lists (CBUF2 *cbuf2)
{
    LIST *p;

    debug_printf ("---CBUF_DEBUG_META_LIST---\n");
    debug_printf("%ld: <size>\n", cbuf2->size);
    debug_printf("%ld: Start\n", cbuf2->base_idx);
    list_for_each (p, &(cbuf2->metadata_list)) {
	METADATA_LIST *meta_tmp;
	int metadata_pos;
	meta_tmp = list_entry (p, METADATA_LIST, m_list);
	metadata_pos = (meta_tmp->m_chunk + 1) * cbuf2->chunk_size - 1;
	debug_printf("%ld: %d [%d]%s\n", 
		     metadata_pos,
		     meta_tmp->m_chunk, 
		     (int) meta_tmp->m_composed_metadata[0],
		     &meta_tmp->m_composed_metadata[1]);
    }
    debug_printf("%ld: End\n", cbuf2_write_index(cbuf2));

    debug_printf ("---CBUF_DEBUG_OGG_LIST---\n");
    debug_printf("%ld: <size>\n", cbuf2->size);
    debug_printf("%ld: Start\n", cbuf2->base_idx);
    list_for_each (p, &(cbuf2->ogg_page_list)) {
	OGG_PAGE_LIST *ogg_tmp;
	ogg_tmp = list_entry (p, OGG_PAGE_LIST, m_list);
	debug_printf("ogg page = %02x [%ld, %ld, %ld]\n", 
		     ogg_tmp->m_page_flags,
		     ogg_tmp->m_page_start,
		     ogg_tmp->m_page_len,
		     cbuf2_add(cbuf2,ogg_tmp->m_page_start,
			       ogg_tmp->m_page_len));
    }
    if (cbuf2->song_page) {
	debug_printf ("%ld(%ld): Song Page\n", 
		      cbuf2->song_page->m_page_start,
		      cbuf2->song_page_done);
    } else {
	debug_printf ("---: Song Page\n");
    }
    debug_printf ("%ld: End\n", cbuf2_write_index(cbuf2));
    debug_printf ("---\n");
}

error_code
cbuf2_ogg_peek_song (CBUF2 *cbuf2, char* out_buf, 
		     unsigned long buf_size,
		     unsigned long* amt_filled,
		     int* eos)
{
    error_code ret;
    unsigned long song_offset;

    *amt_filled = 0;
    *eos = 0;

    /* First time through, set the song_page */
    if (!cbuf2->song_page) {
	if (cbuf2->ogg_page_list.next) {
	    cbuf2->song_page = list_entry (cbuf2->ogg_page_list.next,
					   OGG_PAGE_LIST, m_list);
	    cbuf2->song_page_done = 0;
	} else {
	    return SR_SUCCESS;
	}
    }

    /* Advance song_page if old one is finished */
    if (cbuf2->song_page_done == cbuf2->song_page->m_page_len) {
	if (cbuf2->song_page->m_list.next == &cbuf2->ogg_page_list) {
	    return SR_SUCCESS;
	} else {
	    cbuf2->song_page = list_entry (cbuf2->song_page->m_list.next,
					   OGG_PAGE_LIST, m_list);
	    cbuf2->song_page_done = 0;
	}
    }

    /* Calculate the amount to return to the caller */
    *amt_filled = cbuf2->song_page->m_page_len - cbuf2->song_page_done;
    if (*amt_filled > buf_size) {
	*amt_filled = buf_size;
    } else {
	if (cbuf2->song_page->m_page_flags & OGG_PAGE_EOS) {
	    *eos = 1;
	}
    }

    song_offset = cbuf2_offset_2 (cbuf2, cbuf2->song_page->m_page_start);
    song_offset += cbuf2->song_page_done;
    ret = cbuf2_peek_rgn (cbuf2, out_buf, song_offset, *amt_filled);
    if (ret != SR_SUCCESS) {
	return ret;
    }

    cbuf2->song_page_done += *amt_filled;
    return SR_SUCCESS;
}

u_long
cbuf2_get_free (CBUF2 *cbuf2)
{
    return cbuf2->size - cbuf2->item_count;
}

/* Returns the amount of contiguous free memory immediately 
   following the write index */
u_long
cbuf2_get_free_tail (CBUF2 *cbuf2)
{
    unsigned long ft = cbuf2->size - cbuf2_write_index(cbuf2);
    unsigned long fr = cbuf2_get_free(cbuf2);
    if (ft > fr) {
	return fr;
    } else {
	return ft;
    }
}

/* Returns the amount of contiguous used memory from read_ptr 
   to end of cbuf, and then from start to end of used. */
static void
cbuf2_get_used_fragments (CBUF2 *cbuf2, u_long* frag1, u_long* frag2)
{
    u_long ut = cbuf2->size - cbuf2->base_idx;
    u_long us = cbuf2->item_count;
    if (us > ut) {
	*frag1 = ut;
	*frag2 = us - ut;
    } else {
	*frag1 = us;
	*frag2 = 0;
    }
}

u_long
cbuf2_write_index (CBUF2 *cbuf2)
{
    return cbuf2_add (cbuf2, cbuf2->base_idx, cbuf2->item_count);
}

static u_long
cbuf2_add (CBUF2 *cbuf2, u_long pos, u_long len)
{
    pos += len;
    if (pos >= cbuf2->size)
	pos -= cbuf2->size;
    return pos;
}

static u_long
cbuf2_offset (CBUF2 *cbuf2, u_long pos)
{
    long diff = pos - cbuf2->base_idx;
    if (diff >= 0) {
	return diff;
    } else {
	return diff + cbuf2->size;
    }
}

/* This is like the above, but it never returns zero */
static u_long
cbuf2_offset_2 (CBUF2 *cbuf2, u_long pos)
{
    long diff = pos - cbuf2->base_idx;
    if (diff > 0) {
	return diff;
    } else {
	return diff + cbuf2->size;
    }
}

/* returns pos1 - pos2 (not used) */
#if defined (commentout)
static u_long
cbuf2_subtract (CBUF2 *cbuf2, u_long pos1, u_long pos2)
{
    long diff = pos1 - pos2;
    if (diff > 0) {
	return diff;
    } else {
	return diff + cbuf2->item_count;
    }
}
#endif

static u_long
cbuf2_idx_to_chunk (CBUF2 *cbuf2, u_long idx)
{
    return idx / cbuf2->chunk_size;
}

error_code
cbuf2_peek_rgn (CBUF2 *cbuf2, char *out_buf, u_long start_offset, 
		u_long length)
{
    u_long sidx;

    if (length > cbuf2->item_count)
	return SR_ERROR_BUFFER_EMPTY;
    sidx = cbuf2_add (cbuf2, cbuf2->base_idx, start_offset);
    if (sidx + length > cbuf2->size) {
	int first_hunk = cbuf2->size - sidx;
	memcpy (out_buf, cbuf2->buf+sidx, first_hunk);
	sidx = 0;
	length -= first_hunk;
	out_buf += first_hunk;
    }
    memcpy (out_buf, cbuf2->buf+sidx, length);

    return SR_SUCCESS;
}

error_code
cbuf2_init_relay_entry (CBUF2 *cbuf2, RELAY_LIST* ptr, u_long burst_request)
{
    int buffer_size;
    LIST *p;
    OGG_PAGE_LIST *tmp;

    threadlib_waitfor_sem (&cbuf2->cbuf_sem);
    if (burst_request >= cbuf2->item_count) {
	ptr->m_cbuf_pos = 0;
    } else {
	ptr->m_cbuf_pos = cbuf2->item_count - burst_request;
    }
    if (cbuf2->content_type == CONTENT_TYPE_OGG) {
	/* Move to ogg page boundary */
	int no_boundary_yet = 1;
	u_long req_pos = ptr->m_cbuf_pos;
	p = cbuf2->ogg_page_list.next;
	list_for_each (p, &(cbuf2->ogg_page_list)) {
	    u_long page_offset;
	    tmp = list_entry(p, OGG_PAGE_LIST, m_list);
	    page_offset = cbuf2_offset (cbuf2, tmp->m_page_start);
	    if (page_offset < req_pos || no_boundary_yet) {
		if (tmp->m_page_flags & OGG_PAGE_BOS) {
		    ptr->m_cbuf_pos = page_offset;
		    ptr->m_header_buf_ptr = 0;
		    ptr->m_header_buf_len = 0;
		    ptr->m_header_buf_off = 0;
		    no_boundary_yet = 0;
		} else if (!(tmp->m_page_flags & OGG_PAGE_2)) {
		    ptr->m_cbuf_pos = page_offset;
		    ptr->m_header_buf_ptr = tmp->m_header_buf_ptr;
		    ptr->m_header_buf_len = tmp->m_header_buf_len;
		    ptr->m_header_buf_off = 0;
		    no_boundary_yet = 0;
		}
	    }
	    debug_printf ("Computing cbuf2_offset: "
			  "req=%d pos=%d bas=%d pag=%d off=%d hbuf=%d\n",
			  req_pos, ptr->m_cbuf_pos, 
			  cbuf2->base_idx, tmp->m_page_start,
			  page_offset, ptr->m_header_buf_ptr);
	    if (!no_boundary_yet && page_offset > req_pos) {
		debug_printf ("Done computing cbuf2_offset\n");
		break;
	    }
	}
	if (no_boundary_yet) {
	    return SR_ERROR_NO_OGG_PAGES_FOR_RELAY;
	}
    } else {
	/* Move to metadata boundary */
	ptr->m_cbuf_pos = (ptr->m_cbuf_pos / cbuf2->chunk_size) 
		* cbuf2->chunk_size;
    }
    debug_printf ("cbuf2_init_relay_entry: %d %d %d %d\n",
		  ptr->m_cbuf_pos, 
		  cbuf2->base_idx, 
		  cbuf2->item_count, 
		  cbuf2->size
		  );

    if (ptr->m_icy_metadata) {
	buffer_size = cbuf2->chunk_size + 16*256;
	ptr->m_buffer = (char*) malloc (sizeof(char)*buffer_size);
	ptr->m_buffer_size = buffer_size;
    } else {
	buffer_size = cbuf2->chunk_size;
	ptr->m_buffer = (char*) malloc (sizeof(char)*buffer_size);
	ptr->m_buffer_size = buffer_size;
    }
    /* GCS FIX: check for memory error */
    threadlib_signal_sem (&cbuf2->cbuf_sem);
    debug_printf ("Setting relay buffer size to %d\n", buffer_size);
    return SR_SUCCESS;
}
