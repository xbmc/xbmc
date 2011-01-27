////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2005 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// metadata.c

// This module handles the metadata structure introduced in WavPack 4.0

#include "wavpack.h"

#include <stdlib.h>
#include <string.h>

#ifdef DEBUG_ALLOC
#define malloc malloc_db
#define realloc realloc_db
#define free free_db
void *malloc_db (uint32_t size);
void *realloc_db (void *ptr, uint32_t size);
void free_db (void *ptr);
int32_t dump_alloc (void);
#endif

#if defined(UNPACK) || defined(INFO_ONLY)

int read_metadata_buff (WavpackMetadata *wpmd, uchar *blockbuff, uchar **buffptr)
{
    WavpackHeader *wphdr = (WavpackHeader *) blockbuff;
    uchar *buffend = blockbuff + wphdr->ckSize + 8;

    if (buffend - *buffptr < 2)
	return FALSE;

    wpmd->id = *(*buffptr)++;
    wpmd->byte_length = *(*buffptr)++ << 1;

    if (wpmd->id & ID_LARGE) {
	wpmd->id &= ~ID_LARGE;

	if (buffend - *buffptr < 2)
	    return FALSE;

	wpmd->byte_length += *(*buffptr)++ << 9; 
	wpmd->byte_length += *(*buffptr)++ << 17;
    }

    if (wpmd->id & ID_ODD_SIZE) {
	wpmd->id &= ~ID_ODD_SIZE;
	wpmd->byte_length--;
    }

    if (wpmd->byte_length) {
	if (buffend - *buffptr < wpmd->byte_length + (wpmd->byte_length & 1)) {
	    wpmd->data = NULL;
	    return FALSE;
	}

	wpmd->data = *buffptr;
	(*buffptr) += wpmd->byte_length + (wpmd->byte_length & 1);
    }
    else
	wpmd->data = NULL;

    return TRUE;
}

int process_metadata (WavpackContext *wpc, WavpackMetadata *wpmd)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];

    switch (wpmd->id) {
	case ID_DUMMY:
	    return TRUE;

	case ID_DECORR_TERMS:
	    return read_decorr_terms (wps, wpmd);

	case ID_DECORR_WEIGHTS:
	    return read_decorr_weights (wps, wpmd);

	case ID_DECORR_SAMPLES:
	    return read_decorr_samples (wps, wpmd);

	case ID_ENTROPY_VARS:
	    return read_entropy_vars (wps, wpmd);

	case ID_HYBRID_PROFILE:
	    return read_hybrid_profile (wps, wpmd);

	case ID_SHAPING_WEIGHTS:
	    return read_shaping_info (wps, wpmd);

	case ID_FLOAT_INFO:
	    return read_float_info (wps, wpmd);

	case ID_INT32_INFO:
	    return read_int32_info (wps, wpmd);

	case ID_CHANNEL_INFO:
	    return read_channel_info (wpc, wpmd);

	case ID_CONFIG_BLOCK:
	    return read_config_info (wpc, wpmd);

	case ID_WV_BITSTREAM:
	    return init_wv_bitstream (wps, wpmd);

	case ID_WVC_BITSTREAM:
	    return init_wvc_bitstream (wps, wpmd);

	case ID_WVX_BITSTREAM:
	    return init_wvx_bitstream (wps, wpmd);

	case ID_RIFF_HEADER: case ID_RIFF_TRAILER:
	    return read_wrapper_data (wpc, wpmd);

	case ID_MD5_CHECKSUM:
	    if (wpmd->byte_length == 16) {
		memcpy (wpc->config.md5_checksum, wpmd->data, 16);
		wpc->config.flags |= CONFIG_MD5_CHECKSUM;
		wpc->config.md5_read = 1;
	    }

	    return TRUE;

	default:
	    return (wpmd->id & ID_OPTIONAL_DATA) ? TRUE : FALSE;
    }
}

#endif

#ifdef PACK

int copy_metadata (WavpackMetadata *wpmd, uchar *buffer_start, uchar *buffer_end)
{
    uint32_t mdsize = wpmd->byte_length + (wpmd->byte_length & 1);
    WavpackHeader *wphdr = (WavpackHeader *) buffer_start;

    if (wpmd->byte_length & 1)
	((char *) wpmd->data) [wpmd->byte_length] = 0;

    mdsize += (wpmd->byte_length > 510) ? 4 : 2;
    buffer_start += wphdr->ckSize + 8;

    if (buffer_start + mdsize >= buffer_end)
	return FALSE;

    buffer_start [0] = wpmd->id | (wpmd->byte_length & 1 ? ID_ODD_SIZE : 0);
    buffer_start [1] = (wpmd->byte_length + 1) >> 1;

    if (wpmd->byte_length > 510) {
	buffer_start [0] |= ID_LARGE;
	buffer_start [2] = (wpmd->byte_length + 1) >> 9;
	buffer_start [3] = (wpmd->byte_length + 1) >> 17;
    }

    if (wpmd->data && wpmd->byte_length) {
	if (wpmd->byte_length > 510) {
	    buffer_start [0] |= ID_LARGE;
	    buffer_start [2] = (wpmd->byte_length + 1) >> 9;
	    buffer_start [3] = (wpmd->byte_length + 1) >> 17;
	    memcpy (buffer_start + 4, wpmd->data, mdsize - 4);
	}
	else
	    memcpy (buffer_start + 2, wpmd->data, mdsize - 2);
    }

    wphdr->ckSize += mdsize;
    return TRUE;
}

int add_to_metadata (WavpackContext *wpc, void *data, uint32_t bcount, uchar id)
{
    WavpackMetadata *mdp;
    uchar *src = data;

    while (bcount) {
	if (wpc->metacount) {
	    uint32_t bc = bcount;

	    mdp = wpc->metadata + wpc->metacount - 1;

	    if (mdp->id == id) {
		if (wpc->metabytes + bcount > 1000000)
		    bc = 1000000 - wpc->metabytes;

		mdp->data = realloc (mdp->data, mdp->byte_length + bc);
		memcpy ((char *) mdp->data + mdp->byte_length, src, bc);
		mdp->byte_length += bc;
		wpc->metabytes += bc;
		bcount -= bc;
		src += bc;

		if (wpc->metabytes >= 1000000 && !write_metadata_block (wpc))
		    return FALSE;
	    }
	}

	if (bcount) {
	    wpc->metadata = realloc (wpc->metadata, (wpc->metacount + 1) * sizeof (WavpackMetadata));
	    mdp = wpc->metadata + wpc->metacount++;
	    mdp->byte_length = 0;
	    mdp->data = NULL;
	    mdp->id = id;
	}
    }

    return TRUE;
}

static char *write_metadata (WavpackMetadata *wpmd, char *outdata)
{
    uchar id = wpmd->id, wordlen [3];

    wordlen [0] = (wpmd->byte_length + 1) >> 1;
    wordlen [1] = (wpmd->byte_length + 1) >> 9;
    wordlen [2] = (wpmd->byte_length + 1) >> 17;

    if (wpmd->byte_length & 1) {
//	((char *) wpmd->data) [wpmd->byte_length] = 0;
	id |= ID_ODD_SIZE;
    }

    if (wordlen [1] || wordlen [2])
	id |= ID_LARGE;

    *outdata++ = id;
    *outdata++ = wordlen [0];

    if (id & ID_LARGE) {
	*outdata++ = wordlen [1];
	*outdata++ = wordlen [2];
    }

    if (wpmd->data && wpmd->byte_length) {
	memcpy (outdata, wpmd->data, wpmd->byte_length);
	outdata += wpmd->byte_length;

	if (wpmd->byte_length & 1)
	    *outdata++ = 0;
    }

    return outdata;
}

int write_metadata_block (WavpackContext *wpc)
{
    char *block_buff, *block_ptr;
    WavpackHeader *wphdr;

    if (wpc->metacount) {
	int metacount = wpc->metacount, block_size = sizeof (WavpackHeader);
	WavpackMetadata *wpmdp = wpc->metadata;

	while (metacount--) {
	    block_size += wpmdp->byte_length + (wpmdp->byte_length & 1);
	    block_size += (wpmdp->byte_length > 510) ? 4 : 2;
	    wpmdp++;
	}

	wphdr = (WavpackHeader *) (block_buff = malloc (block_size));

	CLEAR (*wphdr);
	memcpy (wphdr->ckID, "wvpk", 4);
	wphdr->total_samples = wpc->total_samples;
	wphdr->version = 0x403;
	wphdr->ckSize = block_size - 8;
	wphdr->block_samples = 0;

	block_ptr = (char *)(wphdr + 1);

	wpmdp = wpc->metadata;

	while (wpc->metacount) {
	    block_ptr = write_metadata (wpmdp, block_ptr);
	    wpc->metabytes -= wpmdp->byte_length;
	    free_metadata (wpmdp++);
	    wpc->metacount--;
	}

	free (wpc->metadata);
	wpc->metadata = NULL;
	native_to_little_endian ((WavpackHeader *) block_buff, WavpackHeaderFormat);

	if (!wpc->blockout (wpc->wv_out, block_buff, block_size)) {
	    free (block_buff);
	    strcpy (wpc->error_message, "can't write WavPack data, disk probably full!");
	    return FALSE;
	}

	free (block_buff);
    }

    return TRUE;
}

#endif

void free_metadata (WavpackMetadata *wpmd)
{
    if (wpmd->data) {
	free (wpmd->data);
	wpmd->data = NULL;
    }
}
