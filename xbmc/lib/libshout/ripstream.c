/* ripstream.c
 * buffer stream data, when a track changes decodes the audio and 
 * finds a silent point to split the track
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
#ifndef _WIN32
#include <sys/types.h>
#include <netinet/in.h>
#endif
#include "srtypes.h"
#include "cbuf2.h"
#include "findsep.h"
#include "mchar.h"
#include "parse.h"
#include "rip_manager.h"
#include "ripstream.h"
#include "debug.h"
#include "filelib.h"
#include "relaylib.h"
#include "socklib.h"
#include "external.h"
#include "ripogg.h"

/*****************************************************************************
 * Private functions
 *****************************************************************************/
static error_code find_sep (u_long *pos1, u_long *pos2);
static error_code start_track_mp3 (TRACK_INFO* ti);
static error_code end_track_mp3 (RIP_MANAGER_OPTIONS* rmo, u_long pos1, u_long pos2, TRACK_INFO* ti);
static error_code end_track_ogg (RIP_MANAGER_OPTIONS* rmo, TRACK_INFO* ti);

static void compute_cbuf2_size (SPLITPOINT_OPTIONS *sp_opt,
				  int bitrate, int meta_interval);
static int ms_to_bytes (int ms, int bitrate);
static int bytes_to_secs (unsigned int bytes);
static void clear_track_info (TRACK_INFO* ti);
static int ripstream_recvall (char* buffer, int size);

static error_code get_track_from_metadata (int size, char *newtrack);
static error_code get_stream_data (char *data_buf, char *track_buf);
static error_code ripstream_rip_mp3 (RIP_MANAGER_OPTIONS* rmo);
static error_code ripstream_rip_ogg (RIP_MANAGER_OPTIONS* rmo);

/*****************************************************************************
 * Private Vars
 *****************************************************************************/
#define DEFAULT_BUFFER_SIZE	1024

static TRACK_INFO m_old_track;	    /* The track that's being ripped now */
static TRACK_INFO m_new_track;	    /* The track that's gonna start soon */
static TRACK_INFO m_current_track;  /* The metadata as I'm parsing it */
static int m_first_time_through;
static char			m_no_meta_name[MAX_TRACK_LEN] = {'\0'};
static char			*m_getbuffer = NULL;
static int			m_find_silence = -1;
//static BOOL			m_addID3tag = TRUE;
static BOOL			m_add_id3v1 = TRUE;
static BOOL			m_add_id3v2 = TRUE;
static SPLITPOINT_OPTIONS	*m_sp_opt;
static int			m_bitrate;
static int			m_http_bitrate;
static int			m_meta_interval;
static unsigned int		m_cue_sheet_bytes = 0;
static External_Process*        m_external_process = 0;

static int m_cbuf2_size;             /* blocks */
static int m_rw_start_to_cb_end;     /* bytes */
static int m_rw_start_to_sw_start;   /* milliseconds */
static int m_rw_end_to_cb_end;       /* bytes */
static int m_mic_to_cb_end;          /* blocks */

static int m_drop_count;
static int m_track_count = 0;
static int m_content_type;
static HSOCKET m_sock;
static int m_have_relay;
static int m_timeout;

static int m_meta_interval;
static unsigned long m_buffersize;
static int m_chunkcount;

typedef struct ID3V1st
{
        char    tag[3];
        char    songtitle[30];
        char    artist[30];
        char    album[30];
        char    year[4];
        char    comment[30];
        char    genre;
} ID3V1Tag;

#if defined (commentout)
typedef struct ID3V2headst {
	char	tag[3];
	int	version;
	char	flags;
	int	size;
} ID3V2head;
#endif

typedef struct ID3V2framest {
	char	id[4];
	int	size;
	char	pad[3];
} ID3V2frame;


error_code
ripstream_init (HSOCKET sock, 
		int have_relay,
		int timeout, 
		char *no_meta_name, 
		int drop_count,
		SPLITPOINT_OPTIONS *sp_opt, 
		int bitrate, 
		int meta_interval, 
		int content_type, 
		BOOL add_id3v1,
		BOOL add_id3v2,
		External_Process* ep)
{
    if (!sp_opt || !no_meta_name) {
	printf ("Error: invalid ripstream parameters\n");
	return SR_ERROR_INVALID_PARAM;
    }

    m_sock = sock;
    m_have_relay = have_relay;
    m_timeout = timeout;
    m_sp_opt = sp_opt;
    m_track_count = 0;
    m_add_id3v1 = add_id3v1;
    m_add_id3v2 = add_id3v2;
    strcpy(m_no_meta_name, no_meta_name);
    m_drop_count = drop_count;
    m_http_bitrate = bitrate;
    m_bitrate = -1;
    m_content_type = content_type;
    /* GCS RMK: Ripchunk_size is the metaint size, or default size
       if stream doesn't have meta data */
    m_meta_interval = meta_interval;
    m_cue_sheet_bytes = 0;
    m_external_process = ep;

    /* From ripshout */
    m_buffersize = (m_meta_interval == NO_META_INTERVAL) 
	    ? DEFAULT_BUFFER_SIZE : m_meta_interval;

    m_chunkcount = 0;

    clear_track_info (&m_old_track);
    clear_track_info (&m_new_track);
    clear_track_info (&m_current_track);
    m_first_time_through = 1;

    if ((m_getbuffer = malloc(m_buffersize)) == NULL)
	return SR_ERROR_CANT_ALLOC_MEMORY;

    return SR_SUCCESS;
}

void
ripstream_destroy()
{
    debug_printf ("RIPSTREAM_DESTROY\n");
    if (m_getbuffer) {free(m_getbuffer); m_getbuffer = NULL;}
    m_find_silence = -1;
    m_cbuf2_size = 0;
    cbuf2_destroy (&g_cbuf2);

    clear_track_info (&m_old_track);
    clear_track_info (&m_new_track);
    clear_track_info (&m_current_track);
    m_first_time_through = 1;

    m_no_meta_name[0] = '\0';
    m_track_count = 0;
    m_add_id3v1 = TRUE;
    m_add_id3v2 = TRUE;

    /* from ripshout */
    m_buffersize = 0;
    m_meta_interval = 0;
    m_chunkcount = 0;
}

BOOL
is_track_changed()
{
    /* If metadata is duplicate of previous, then no change. */
    if (!strcmp(m_old_track.raw_metadata, m_current_track.raw_metadata))
	return 0;

    /* Otherwise, there was a change. */
    return 1;
}

static void
format_track_info (TRACK_INFO* ti, char* tag)
{
    debug_mprintf (m_("----- TRACK_INFO ") m_s m_("\n")
		   m_("HAVETI: %d\n")
		   m_("RAW_MD: ") m_s m_("\n")
		   m_("ARTIST: ") m_S m_("\n")
		   m_("TITLE:  ") m_S m_("\n")
		   m_("ALBUM:  ") m_S m_("\n")
		   m_("SAVE:   %d\n"),
		   tag,
		   ti->have_track_info,
		   ti->raw_metadata,
		   ti->artist,
		   ti->title,
		   ti->album,
		   ti->save_track);
}

static void
clear_track_info (TRACK_INFO* ti)
{
    ti->have_track_info = 0;
    ti->raw_metadata[0] = 0;
    ti->artist[0] = 0;
    ti->title[0] = 0;
    ti->album[0] = 0;
    ti->track[0] = 0;
    ti->composed_metadata[0] = 0;
    ti->save_track = TRUE;
}

static void
copy_track_info (TRACK_INFO* dest, TRACK_INFO* src)
{
    dest->have_track_info = src->have_track_info;
    strcpy (dest->raw_metadata, src->raw_metadata);
    mstrcpy (dest->artist, src->artist);
    mstrcpy (dest->title, src->title);
    mstrcpy (dest->album, src->album);
    strcpy (dest->composed_metadata, src->composed_metadata);
    dest->save_track = src->save_track;
}

/**** The main loop for ripping ****/
error_code
ripstream_rip (RIP_MANAGER_OPTIONS* rmo)
{
    if (m_content_type == CONTENT_TYPE_OGG) {
	return ripstream_rip_ogg (rmo);
    } else {
	return ripstream_rip_mp3 (rmo);
    }
}

static error_code
ripstream_rip_ogg (RIP_MANAGER_OPTIONS* rmo)
{
    int ret;
    int real_ret = SR_SUCCESS;
    u_long extract_size;
    static int have_track = 0;

    /* get the data from the stream */
    debug_printf ("RIPSTREAM_RIP_OGG: top of loop\n");
    ret = get_stream_data(m_getbuffer, m_current_track.raw_metadata);
    if (ret != SR_SUCCESS) {
	debug_printf("get_stream_data bad return code: %d\n", ret);
	return ret;
    }

    if (m_first_time_through) {
	/* Allocate circular buffer */
	m_bitrate = -1;
	m_buffersize = 1024;
	m_cbuf2_size = 128;
	ret = cbuf2_init (&g_cbuf2, m_content_type, m_have_relay, 
			  m_buffersize, m_cbuf2_size);
	if (ret != SR_SUCCESS) return ret;
	have_track = 0;
	/* Warm up the ogg decoding routines */
	rip_ogg_init ();
	/* Done! */
	m_first_time_through = 0;
    }

    /* Copy the data into cbuffer */
    clear_track_info (&m_current_track);
    ret = cbuf2_insert_chunk (&g_cbuf2, m_getbuffer, m_buffersize,
			      m_content_type, &m_current_track);
    if (ret != SR_SUCCESS) {
	debug_printf("start_track had bad return code %d\n", ret);
	return ret;
    }

    filelib_write_show (m_getbuffer, m_buffersize);

    /* If we have unwritten pages for the current track, write them */
    if (have_track) {
	do {
	    error_code ret;
	    unsigned long amt_filled;
	    int got_eos;
	    ret = cbuf2_ogg_peek_song (&g_cbuf2, m_getbuffer, m_buffersize,
				       &amt_filled, &got_eos);
	    debug_printf ("^^^ogg_peek: %d %d\n", amt_filled, got_eos);
	    if (ret != SR_SUCCESS) {
		debug_printf ("cbuf2_ogg_peek_song: %d\n",ret);
		return ret;
	    }
	    if (amt_filled == 0) {
		break;
	    }
	    ret = rip_manager_put_data (m_getbuffer, amt_filled);
	    if (ret != SR_SUCCESS) {
		debug_printf ("rip_manager_put_data(#1): %d\n",ret);
		return ret;
	    }
	    if (got_eos) {
		end_track_ogg (rmo, &m_old_track);
		have_track = 0;
		break;
	    }
	} while (1);
    }

    format_track_info (&m_current_track, "current");

    /* If we got a new track, then start a new file */
    if (m_current_track.have_track_info) {
	ret = rip_manager_start_track (&m_current_track, m_track_count);
	if (ret != SR_SUCCESS) {
	    debug_printf ("rip_manager_start_track failed(#1): %d\n",ret);
	    return ret;
	}
	filelib_write_cue (&m_current_track, 0);
	copy_track_info (&m_old_track, &m_current_track);

	have_track = 1;
    }

    /* If buffer almost full, advance the buffer */
    if (cbuf2_get_free(&g_cbuf2) < m_buffersize) {
	debug_printf ("cbuf2_get_free < m_buffersize\n");
	extract_size = m_buffersize - cbuf2_get_free(&g_cbuf2);

        ret = cbuf2_advance_ogg (&g_cbuf2, m_buffersize);

        if (ret != SR_SUCCESS) {
	    debug_printf("cbuf2_extract had bad return code %d\n", ret);
	    return ret;
	}
    }

    return real_ret;
}

static error_code
ripstream_rip_mp3 (RIP_MANAGER_OPTIONS* rmo)
{
    int ret;
    int real_ret = SR_SUCCESS;
    u_long extract_size;

    /* get the data & meta-data from the stream */
    debug_printf ("RIPSTREAM_RIP: top of loop\n");
    ret = get_stream_data(m_getbuffer, m_current_track.raw_metadata);
    if (ret != SR_SUCCESS) {
	debug_printf("get_stream_data bad return code: %d\n", ret);
	return ret;
    }

    /* First time through, need to determine the bitrate. 
       The bitrate is needed to do the track splitting parameters 
       properly in seconds.  See the readme file for details.  */
    /* GCS FIX: For VBR streams, the header value may be more reliable. */
    if (m_first_time_through) {
        unsigned long test_bitrate;
	debug_printf("Querying stream for bitrate - first time.\n");
	if (m_content_type == CONTENT_TYPE_MP3) {
	    find_bitrate(&test_bitrate, m_getbuffer, m_buffersize);
	    m_bitrate = test_bitrate / 1000;
	    debug_printf("Got bitrate: %d\n",m_bitrate);
	} else {
	    m_bitrate = 0;
	}

	if (m_bitrate == 0) {
	    /* Couldn't decode from mp3, so let's go with what the 
	       http header says, or fallback to 24.  */
	    if (m_http_bitrate > 0)
		m_bitrate = m_http_bitrate;
	    else
		m_bitrate = 24;
	}
        compute_cbuf2_size (m_sp_opt, m_bitrate, m_buffersize);
	ret = cbuf2_init (&g_cbuf2, m_content_type, m_have_relay, 
			  m_buffersize, m_cbuf2_size);
	if (ret != SR_SUCCESS) return ret;
    }

    if (m_external_process) {
	/* If using external metadata, check for that */
	clear_track_info (&m_current_track);
	read_external (m_external_process, &m_current_track);
    } else {
	if (m_current_track.raw_metadata[0]) {
	    parse_metadata (&m_current_track);
	} else {
	    clear_track_info (&m_current_track);
	}
    }

    /* Copy the data into cbuffer */
    ret = cbuf2_insert_chunk (&g_cbuf2, m_getbuffer, m_buffersize,
			      m_content_type, &m_current_track);
    if (ret != SR_SUCCESS) {
	debug_printf("cbuf2_insert_chunk had bad return code %d\n", ret);
	return ret;
    }

    filelib_write_show (m_getbuffer, m_buffersize);

    /* First time through, so start a track. */
    if (m_first_time_through) {
	int ret;
	debug_printf ("First time through...\n");
	m_first_time_through = 0;
	if (!m_current_track.have_track_info) {
	    strcpy (m_current_track.raw_metadata, m_no_meta_name);
	}
	ret = rip_manager_start_track (&m_current_track, m_track_count);
	if (ret != SR_SUCCESS) {
	    debug_printf ("rip_manager_start_track failed(#1): %d\n",ret);
	    return ret;
	}
	filelib_write_cue (&m_current_track, 0);
	copy_track_info (&m_old_track, &m_current_track);
    }

    /* Check for track change. */
    debug_printf ("m_current_track.have_track_info = %d\n", 
		  m_current_track.have_track_info);
    if (m_current_track.have_track_info && is_track_changed()) {
	/* Set m_find_silence equal to the number of additional blocks 
	   needed until we can do silence separation. */
	debug_printf ("VERIFIED TRACK CHANGE (m_find_silence = %d)\n",
		      m_find_silence);
	copy_track_info (&m_new_track, &m_current_track);
	if (m_find_silence < 0) {
	    if (m_mic_to_cb_end > 0) {
		m_find_silence = m_mic_to_cb_end;
	    } else {
		m_find_silence = 0;
	    }
	}
    }

    format_track_info (&m_old_track, "old");
    format_track_info (&m_new_track, "new");
    format_track_info (&m_current_track, "current");

    if (m_find_silence == 0) {
	/* Find separation point */
	u_long pos1, pos2;
	debug_printf ("m_find_silence == 0\n");
	ret = find_sep (&pos1, &pos2);
	if (ret == SR_ERROR_REQUIRED_WINDOW_EMPTY) {
	    /* If this happens, the previous song should be truncated to 
	       zero bytes. */
	    pos1 = 0;
	    pos2 = 0;
	}
	else if (ret != SR_SUCCESS) {
	    debug_printf("find_sep had bad return code %d\n", ret);
	    return ret;
	}

	/* Write out previous track */
	ret = end_track_mp3 (rmo, pos1, pos2, &m_old_track);
	if (ret != SR_SUCCESS)
	    real_ret = ret;
	m_cue_sheet_bytes += pos2;

	/* Start next track */
	ret = start_track_mp3 (&m_new_track);
	if (ret != SR_SUCCESS)
	    real_ret = ret;
	m_find_silence = -1;

	copy_track_info (&m_old_track, &m_new_track);
    }
    if (m_find_silence >= 0) m_find_silence --;

    /* If buffer almost full, dump extra to current song. */
    if (cbuf2_get_free(&g_cbuf2) < m_buffersize) {
	u_long curr_song;
	debug_printf ("cbuf2_get_free < m_buffersize\n");
	extract_size = m_buffersize - cbuf2_get_free(&g_cbuf2);
        ret = cbuf2_extract(&g_cbuf2, m_getbuffer, extract_size, &curr_song);
        if (ret != SR_SUCCESS) {
	    debug_printf("cbuf2_extract had bad return code %d\n", ret);
	    return ret;
	}

	/* Post to caller */
	if (curr_song < extract_size) {
	    u_long curr_song_bytes = extract_size - curr_song;
	    m_cue_sheet_bytes += curr_song_bytes;
	    rip_manager_put_data (&m_getbuffer[curr_song], curr_song_bytes);
	}
    }

    return real_ret;
}

error_code
find_sep (u_long *pos1, u_long *pos2)
{
    int rw_start, rw_end, sw_sil;
    int ret;

    debug_printf ("*** Finding separation point\n");

    /* First, find the search region w/in cbuffer. */
    rw_start = g_cbuf2.item_count - m_rw_start_to_cb_end;
    if (rw_start < 0) {
	return SR_ERROR_REQUIRED_WINDOW_EMPTY;
    }
    rw_end = g_cbuf2.item_count - m_rw_end_to_cb_end;
    if (rw_end < 0) {
	return SR_ERROR_REQUIRED_WINDOW_EMPTY;
    }

    debug_printf ("search window (bytes): %d,%d,%d\n", rw_start, rw_end,
		  g_cbuf2.item_count);

    if (m_content_type != CONTENT_TYPE_MP3) {
	sw_sil = (rw_end + rw_start) / 2;
	debug_printf ("(not mp3) taking middle: sw_sil=%d\n", sw_sil);
	*pos1 = rw_start + sw_sil;
	*pos2 = rw_start + sw_sil;
    } else {
	int bufsize = rw_end - rw_start;
	char* buf = (u_char *)malloc(bufsize);
	ret = cbuf2_peek_rgn (&g_cbuf2, buf, rw_start, bufsize);
	if (ret != SR_SUCCESS) {
	    debug_printf ("PEEK FAILED: %d\n", ret);
	    free(buf);
	    return ret;
	}
	debug_printf ("PEEK OK\n");

	/* Find silence point */
	ret = findsep_silence (buf, 
			       bufsize, 
			       m_rw_start_to_sw_start,
			       m_sp_opt->xs_search_window_1 
				+ m_sp_opt->xs_search_window_2,
			       m_sp_opt->xs_silence_length,
			       m_sp_opt->xs_padding_1,
			       m_sp_opt->xs_padding_2,
			       pos1, pos2);
	*pos1 += rw_start;
	*pos2 += rw_start;
	free(buf);
    }
    return SR_SUCCESS;
}

static error_code
end_track_mp3 (RIP_MANAGER_OPTIONS* rmo, u_long pos1, u_long pos2, TRACK_INFO* ti)
{
    // pos1 is end of prev track
    // pos2 is beginning of next track
    int ret;
    u_char *buf;

    /* GCS pos1 is byte position. Here we convert it into a "count". */
    pos1++;

    // I think pos can be zero if the silence is right at the beginning
    // i.e. it is a bug in s.r.
    buf = (u_char*) malloc (pos1);

    // pos1 is end of prev track
    // pos2 is beginning of next track
    // positions are relative to cbuf2->read_index

    // First, dump the part only in prev track
    ret = cbuf2_peek(&g_cbuf2, buf, pos1);
    if (ret != SR_SUCCESS) goto BAIL;

    // Let cbuf know about the start of the next track
    cbuf2_set_next_song (&g_cbuf2, pos2);

    // Write that out to the current file
    // GCS FIX: m_bytes_ripped is incorrect when there is padding
    if ((ret = rip_manager_put_data(buf, pos1)) != SR_SUCCESS)
	goto BAIL;

    /* This is id3v1 */
    if (m_add_id3v1) {
	ID3V1Tag id3;
	memset (&id3, '\000',sizeof(id3));
	strncpy (id3.tag, "TAG", strlen("TAG"));
	string_from_mstring (id3.artist, sizeof(id3.artist), 
			     ti->artist, CODESET_ID3);
	string_from_mstring (id3.songtitle, sizeof(id3.songtitle), 
			     ti->title, CODESET_ID3);
	string_from_mstring (id3.album, sizeof(id3.album),
			     ti->album, CODESET_ID3);
	id3.genre = (char) 0xFF; // see http://www.id3.org/id3v2.3.0.html#secA
	ret = rip_manager_put_data ((char *)&id3, sizeof(id3));
	if (ret != SR_SUCCESS) {
	    goto BAIL;
	}
    }

    // Only save this track if we've skipped over enough cruft 
    // at the beginning of the stream
    debug_printf("Current track number %d (skipping if %d or less)\n", 
		 m_track_count, m_drop_count);
    if (m_track_count > m_drop_count)
	if ((ret = rip_manager_end_track (rmo, ti)) != SR_SUCCESS)
	    goto BAIL;

 BAIL:
    free(buf);
    return ret;
}

static error_code
start_track_mp3 (TRACK_INFO* ti)
{
#define HEADER_SIZE 1600
    int ret;
    int i;
    unsigned int secs;

    debug_printf ("calling rip_manager_start_track(#2)\n");
    ret = rip_manager_start_track (ti, m_track_count);
    if (ret != SR_SUCCESS) {
	debug_printf ("rip_manager_start_track failed(#2): %d\n",ret);
        return ret;
    }

    /* Dump to artist/title to cue sheet */
    secs = bytes_to_secs (m_cue_sheet_bytes);
    ret = filelib_write_cue (ti, secs);
    if (ret != SR_SUCCESS)
        return ret;

    /* Oddsock's ID3 stuff, (oddsock@oddsock.org) */
    if (m_add_id3v2) {
	int rc;
	char bigbuf[HEADER_SIZE] = "";
	int header_size = HEADER_SIZE;
	char header1[6] = "ID3\x03\0\0";
	ID3V2frame id3v2frame;
	char comment[1024] = "Ripped with Streamripper";
	unsigned long int framesize = 0;
	int sent = 0;
	int id3_charset;

	memset(bigbuf, '\000', sizeof(bigbuf));

	/* Write header */
	ret = rip_manager_put_data(header1, 6);
	if (ret != SR_SUCCESS) return ret;
	for (i = 0; i < 4; i++) {
	    char x = (header_size >> (3-i)*7) & 0x7F;
	    ret = rip_manager_put_data((char *)&x, 1);
	    if (ret != SR_SUCCESS) return ret;
	}

	/* ID3 V2.3 is only defined for ISO-8859-1 and UCS-2
	   If user specifies another codeset, we will use it, and 
	   report ISO-8859-1 in the encoding field */
	id3_charset = is_id3_unicode();

	/* Write ID3V2 TPE1 frame (Lead performer) */
	memset(&id3v2frame, '\000', sizeof(id3v2frame));
	strncpy(id3v2frame.id, "TPE1", 4);
	id3v2frame.pad[2] = id3_charset;
	rc = string_from_mstring (bigbuf, HEADER_SIZE, ti->artist, 
				  CODESET_ID3);
	framesize = htonl (rc+1);
	ret = rip_manager_put_data ((char *)&(id3v2frame.id), 4);
	if (ret != SR_SUCCESS) return ret;
	sent += 4;
	ret = rip_manager_put_data ((char *)&(framesize), sizeof(framesize));
	if (ret != SR_SUCCESS) return ret;
	sent += sizeof(framesize);
	ret = rip_manager_put_data ((char *)&(id3v2frame.pad), 3);
	if (ret != SR_SUCCESS) return ret;
	sent += 3;
	ret = rip_manager_put_data (bigbuf, rc);
	if (ret != SR_SUCCESS) return ret;
	sent += rc;

	/* Write ID3V2 TIT2 frame (Title) */
	memset(&id3v2frame, '\000', sizeof(id3v2frame));
	strncpy(id3v2frame.id, "TIT2", 4);
	id3v2frame.pad[2] = id3_charset;
	rc = string_from_mstring (bigbuf, HEADER_SIZE, ti->title, 
				  CODESET_ID3);
	framesize = htonl (rc+1);
	ret = rip_manager_put_data((char *)&(id3v2frame.id), 4);
	if (ret != SR_SUCCESS) return ret;
	sent += 4;
	ret = rip_manager_put_data((char *)&(framesize), sizeof(framesize));
	if (ret != SR_SUCCESS) return ret;
	sent += sizeof(framesize);
	ret = rip_manager_put_data((char *)&(id3v2frame.pad), 3);
	if (ret != SR_SUCCESS) return ret;
	sent += 3;
	ret = rip_manager_put_data (bigbuf, rc);
	if (ret != SR_SUCCESS) return ret;
	sent += rc;

	/* Write ID3V2 TENC frame (Encoded by) */
	memset(&id3v2frame, '\000', sizeof(id3v2frame));
	strncpy(id3v2frame.id, "TENC", 4);
	framesize = htonl(strlen(comment)+1);
	ret = rip_manager_put_data((char *)&(id3v2frame.id), 4);
	if (ret != SR_SUCCESS) return ret;
	sent += 4;
	ret = rip_manager_put_data((char *)&(framesize), sizeof(framesize));
	if (ret != SR_SUCCESS) return ret;
	sent += sizeof(framesize);
	ret = rip_manager_put_data((char *)&(id3v2frame.pad), 3);
	if (ret != SR_SUCCESS) return ret;
	sent += 3;
	ret = rip_manager_put_data(comment, strlen(comment));
	if (ret != SR_SUCCESS) return ret;
	sent += strlen (comment);

	/* Write ID3V2 TALB frame (Album) */
	memset(&id3v2frame, '\000', sizeof(id3v2frame));
	strncpy(id3v2frame.id, "TALB", 4);
	id3v2frame.pad[2] = id3_charset;
	rc = string_from_mstring (bigbuf, HEADER_SIZE, ti->album, 
				  CODESET_ID3);
	framesize = htonl (rc+1);
	ret = rip_manager_put_data((char *)&(id3v2frame.id), 4);
	if (ret != SR_SUCCESS) return ret;
	sent += 4;
	ret = rip_manager_put_data((char *)&(framesize), sizeof(framesize));
	if (ret != SR_SUCCESS) return ret;
	sent += sizeof(framesize);
	ret = rip_manager_put_data((char *)&(id3v2frame.pad), 3);
	if (ret != SR_SUCCESS) return ret;
	sent += 3;
	ret = rip_manager_put_data (bigbuf, rc);
	if (ret != SR_SUCCESS) return ret;
	sent += rc;

	/* Zero out padding */
	memset(bigbuf, '\000', sizeof(bigbuf));

	/* Pad up to header_size */
	ret = rip_manager_put_data(bigbuf, HEADER_SIZE-sent);
	if (ret != SR_SUCCESS) return ret;
    }
    m_track_count ++;
    debug_printf ("Changed track count to %d\n", m_track_count);

    return SR_SUCCESS;
}

// Only save this track if we've skipped over enough cruft 
// at the beginning of the stream
static error_code
end_track_ogg (RIP_MANAGER_OPTIONS* rmo, TRACK_INFO* ti)
{
    error_code ret;
    debug_printf ("Current track number %d (skipping if %d or less)\n", 
		  m_track_count, m_drop_count);
    if (m_track_count > m_drop_count) {
	ret = rip_manager_end_track (rmo, ti);
    } else {
	ret = SR_SUCCESS;
    }
    m_track_count ++;
    return ret;
}

#if defined (commentout)
/* GCS: This converts either positive or negative ms to blocks,
   and must work for rounding up and rounding down */
static int
ms_to_blocks (int ms, int bitrate, int round_up)
{
    int ms_abs = ms > 0 ? ms : -ms;
    int ms_sign = ms > 0 ? 1 : 0;
    int bits = ms_abs * bitrate;
    int bits_per_block = 8 * m_buffersize;
    int blocks = bits / bits_per_block;
    if (bits % bits_per_block > 0) {
	if (!(round_up ^ ms_sign)) {
	    blocks++;
	}
    }
    if (!ms_sign) {
	blocks = -blocks;
    }
    return blocks;
}
#endif

/* Simpler routine, rounded toward zero */
static int
ms_to_bytes (int ms, int bitrate)
{
    int bits = ms * bitrate;
    if (bits > 0)
	return bits / 8;
    else
	return -((-bits)/8);
}

/* Assume positive, round toward zero */
static int
bytes_to_secs (unsigned int bytes)
{
    /* divided by 125 because 125 = 1000 / 8 */
    int secs = (bytes / m_bitrate) / 125;
    return secs;
}

/* --------------------------------------------------------------------------
   Buffering for silence splitting & padding

   We may flush the circular buffer as soon as anything that 
   needs to go into the next song has passed.   For simplicity, 
   we also buffer up to the latest point that can go into the 
   next song.  This is called the "required window."

   The entire required window is decoded, even though 
   we don't need the volume.  We simply mark the frame boundaries
   so we don't chop any frames.

   The circular buffer is a bit bigger than the required window, 
   because it contains an integer number of blocks which fall 

   Some abbreviations:
     mic      meta inf change
     cb	      cbuf2, aka circular buffer
     rw	      required window
     sw	      search window

   This is the complete picture:

    A---------A---------A----+----B---------B     meta intervals

                                  /mic            meta-inf change (A to B)
                             +--->|
                             /mi                  meta-inf point
                             |
                   |---+-----+------+---|         search window
                       |            |   
                       |            |   
                   |---+---|    |---+---|         silence length
                       |            |
         |-------------|            |-----|
              prepad                postpad

         |--------------------------------|       required window

    |---------------------------------------|     cbuf


                   |<-------------+               mic_to_sw_start
                                  +---->|         mic_to_sw_end
         |<-----------------------+               mic_to_rw_start
                                  +------>|       mic_to_rw_end
    |<----------------------------+               mic_to_cb_start
                                  +-------->|     mic_to_cb_end

   ------------------------------------------------------------------------*/
static void
compute_cbuf2_size (SPLITPOINT_OPTIONS *sp_opt, int bitrate, 
		      int meta_interval)
{
    long sws, sl;
    long mi_to_mic;
    long prepad, postpad;
    long offset;
    long mic_to_sw_start, mic_to_sw_end;
    long mic_to_rw_start, mic_to_rw_end;
    long mic_to_cb_start, mic_to_cb_end;

    if(meta_interval == 0)
      return;

    debug_printf ("---------------------------------------------------\n");
    debug_printf ("xs_search_window: %d,%d\n",
		  sp_opt->xs_search_window_1,sp_opt->xs_search_window_2);
    debug_printf ("xs_silence_length: %d\n", sp_opt->xs_silence_length);
    debug_printf ("xs_padding: %d,%d\n", sp_opt->xs_padding_1,
		  sp_opt->xs_padding_2);
    debug_printf ("xs_offset: %d\n", sp_opt->xs_offset);
    debug_printf ("---------------------------------------------------\n");
    debug_printf ("bitrate = %d, meta_inf = %d\n", bitrate, meta_interval);
    debug_printf ("---------------------------------------------------\n");
    
    /* mi_to_mic is the "half of a meta-inf" from the meta inf 
       change to the previous (non-changed) meta inf */
    mi_to_mic = meta_interval / 2;
    debug_printf ("mi_to_mic: %d\n", mi_to_mic);

    /* compute the search window size (sws) */
    sws = ms_to_bytes (sp_opt->xs_search_window_1, bitrate) 
	    + ms_to_bytes (sp_opt->xs_search_window_2, bitrate);
    debug_printf ("sws: %d\n", sws);

    /* compute the silence length (sl) */
    sl = ms_to_bytes (sp_opt->xs_silence_length, bitrate);
    debug_printf ("sl: %d\n", sl);

    /* compute padding */
    prepad = ms_to_bytes (sp_opt->xs_padding_1, bitrate);
    postpad = ms_to_bytes (sp_opt->xs_padding_2, bitrate);
    debug_printf ("padding: %d %d\n", prepad, postpad);

    /* compute offset */
    offset = ms_to_bytes(sp_opt->xs_offset,bitrate);
    debug_printf ("offset: %d\n", offset);

    /* compute interval from mi to search window */
    mic_to_sw_start = - mi_to_mic + offset 
	    - ms_to_bytes(sp_opt->xs_search_window_1,bitrate);
    mic_to_sw_end = - mi_to_mic + offset 
	    + ms_to_bytes(sp_opt->xs_search_window_2,bitrate);
    debug_printf ("mic_to_sw_start: %d\n", mic_to_sw_start);
    debug_printf ("mic_to_sw_end: %d\n", mic_to_sw_end);

    /* compute interval from mi to required window */
    mic_to_rw_start = mic_to_sw_start + sl / 2 - prepad;
    if (mic_to_rw_start > mic_to_sw_start) {
	mic_to_rw_start = mic_to_sw_start;
    }
    mic_to_rw_end = mic_to_sw_end - sl / 2 + postpad;
    if (mic_to_rw_end < mic_to_sw_end) {
	mic_to_rw_end = mic_to_sw_end;
    }
    debug_printf ("mic_to_rw_start: %d\n", mic_to_rw_start);
    debug_printf ("mic_to_rw_end: %d\n", mic_to_rw_end);

    /* This code replaces the 3 cases (see OBSOLETE in gcs_notes.txt) */
    mic_to_cb_start = mic_to_rw_start;
    mic_to_cb_end = mic_to_rw_end;
    if (mic_to_cb_start > -meta_interval) {
	mic_to_cb_start = -meta_interval;
    }
    if (mic_to_cb_end < 0) {
	mic_to_cb_end = 0;
    }

    /* Convert to chunks & compute cbuf size */
    mic_to_cb_end = (mic_to_cb_end + (meta_interval-1)) / meta_interval;
    mic_to_cb_start = -((-mic_to_cb_start + (meta_interval-1)) 
			/ meta_interval);
    m_cbuf2_size = -mic_to_cb_start + mic_to_cb_end;
    if (m_cbuf2_size < 3) {
	m_cbuf2_size = 3;
    }
    debug_printf ("mic_to_cb_start: %d\n", mic_to_cb_start * meta_interval);
    debug_printf ("mic_to_cb_end: %d\n", mic_to_cb_end * meta_interval);
    debug_printf ("CBUF2 (BLOCKS): %d:%d -> %d\n", mic_to_cb_start,
		  mic_to_cb_end, m_cbuf2_size);

    /* Set some global variables to be used by splitting algorithm */
    m_mic_to_cb_end = mic_to_cb_end;
    m_rw_start_to_cb_end = mic_to_cb_end * meta_interval - mic_to_rw_start;
    m_rw_end_to_cb_end = mic_to_cb_end * meta_interval - mic_to_rw_end;
    m_rw_start_to_sw_start = sp_opt->xs_padding_1 - sp_opt->xs_silence_length / 2;
    if (m_rw_start_to_sw_start < 0) {
	m_rw_start_to_sw_start = 0;
    }
    debug_printf ("m_mic_to_cb_end: %d\n", m_mic_to_cb_end);
    debug_printf ("m_rw_start_to_cb_end: %d\n", m_rw_start_to_cb_end);
    debug_printf ("m_rw_end_to_cb_end: %d\n", m_rw_end_to_cb_end);
    debug_printf ("m_rw_start_to_sw_start: %d\n", m_rw_start_to_sw_start);
}

/* GCS: This used to be myrecv in rip_manager.c */
static int
ripstream_recvall (char* buffer, int size)
{
    int ret;
    /* GCS: Jun 5, 2004.  Here is where I think we are getting aussie's 
       problem with the SR_ERROR_INVALID_METADATA or SR_ERROR_NO_TRACK_INFO
       messages */
    ret = socklib_recvall(&m_sock, buffer, size, m_timeout);
    if (ret >= 0 && ret != size) {
	debug_printf ("rip_manager_recv: expected %d, got %d\n",size,ret);
	ret = SR_ERROR_RECV_FAILED;
    }
    return ret;
}

static error_code
get_stream_data (char *data_buf, char *track_buf)
{
    int ret = 0;
    char c;
    char newtrack[MAX_TRACK_LEN];

    *track_buf = 0;
    m_chunkcount++;
    if ((ret = ripstream_recvall (data_buf, m_buffersize)) <= 0)
	return ret;

    if (m_meta_interval == NO_META_INTERVAL) {
	return SR_SUCCESS;
    }

    if ((ret = ripstream_recvall (&c, 1)) <= 0)
	return ret;

    debug_printf ("METADATA LEN: %d\n",(int)c);
    if (c < 0) {
	debug_printf ("Got invalid metadata: %d\n",c);
	return SR_ERROR_INVALID_METADATA;
    } else if (c == 0) {
	// around christmas time 2001 someone noticed that 1.8.7 shoutcast
	// none-meta servers now just send a '0' if they have no meta data
	// anyway, bassicly if the first meta capture is null then we assume
	// that the stream does not have metadata
	return SR_SUCCESS;
    } else {
	if ((ret = get_track_from_metadata (c * 16, newtrack)) != SR_SUCCESS) {
	    debug_printf("get_trackname had a bad return %d", ret);
	    return ret;
	}

	// WolfFM is a station that does not stream meta-data, but is in that format anyway...
	// StreamTitle='', so we need to pretend this has no meta data.
	if (*newtrack == '\0') {
	    return SR_SUCCESS;
	}
	/* This is the case where we got metadata. */
	strncpy(track_buf, newtrack, MAX_TRACK_LEN);
    }
    return SR_SUCCESS;
}

static error_code
get_track_from_metadata (int size, char *newtrack)
{
    int i;
    int ret;
    char *namebuf;

    if ((namebuf = malloc(size)) == NULL)
	return SR_ERROR_CANT_ALLOC_MEMORY;

    if ((ret = ripstream_recvall (namebuf, size)) <= 0) {
	free(namebuf);
	return ret;
    }

    debug_printf ("METADATA TITLE\n");
    for (i=0; i<size; i++) {
	debug_printf ("%2x ",(unsigned char)namebuf[i]);
	if (i % 20 == 19) {
	    debug_printf ("\n");
	}
    }
    debug_printf ("\n");
    for (i=0; i<size; i++) {
	debug_printf ("%2c ",namebuf[i]);
	if (i % 20 == 19) {
	    debug_printf ("\n");
	}
    }
    debug_printf ("\n");

    if(strstr(namebuf, "StreamTitle='") == NULL) {
	free(namebuf);
	return SR_ERROR_NO_TRACK_INFO;
    }
    subnstr_until(namebuf+strlen("StreamTitle='"), "';", newtrack, MAX_TRACK_LEN);
    trim(newtrack);

    free(namebuf);
    return SR_SUCCESS;
}
