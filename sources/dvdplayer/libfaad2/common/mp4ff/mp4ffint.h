/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: mp4ffint.h,v 1.15 2004/01/14 20:50:22 menno Exp $
**/

#ifndef MP4FF_INTERNAL_H
#define MP4FF_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mp4ff_int_types.h"


#ifdef _WIN32
#define ITUNES_DRM
#endif


#define MAX_TRACKS 1024
#define TRACK_UNKNOWN 0
#define TRACK_AUDIO   1
#define TRACK_VIDEO   2
#define TRACK_SYSTEM  3


#define SUBATOMIC 128

/* atoms without subatoms */
#define ATOM_FTYP 129
#define ATOM_MDAT 130
#define ATOM_MVHD 131
#define ATOM_TKHD 132
#define ATOM_TREF 133
#define ATOM_MDHD 134
#define ATOM_VMHD 135
#define ATOM_SMHD 136
#define ATOM_HMHD 137
#define ATOM_STSD 138
#define ATOM_STTS 139
#define ATOM_STSZ 140
#define ATOM_STZ2 141
#define ATOM_STCO 142
#define ATOM_STSC 143
#define ATOM_MP4A 144
#define ATOM_MP4V 145
#define ATOM_MP4S 146
#define ATOM_ESDS 147
#define ATOM_META 148 /* iTunes Metadata box */
#define ATOM_NAME 149 /* iTunes Metadata name box */
#define ATOM_DATA 150 /* iTunes Metadata data box */
#define ATOM_CTTS 151
#define ATOM_FRMA 152
#define ATOM_IVIV 153
#define ATOM_PRIV 154

#define ATOM_UNKNOWN 255
#define ATOM_FREE ATOM_UNKNOWN
#define ATOM_SKIP ATOM_UNKNOWN

/* atoms with subatoms */
#define ATOM_MOOV 1
#define ATOM_TRAK 2
#define ATOM_EDTS 3
#define ATOM_MDIA 4
#define ATOM_MINF 5
#define ATOM_STBL 6
#define ATOM_UDTA 7
#define ATOM_ILST 8 /* iTunes Metadata list */
#define ATOM_TITLE 9
#define ATOM_ARTIST 10
#define ATOM_WRITER 11
#define ATOM_ALBUM 12
#define ATOM_DATE 13
#define ATOM_TOOL 14
#define ATOM_COMMENT 15
#define ATOM_GENRE1 16
#define ATOM_TRACK 17
#define ATOM_DISC 18
#define ATOM_COMPILATION 19
#define ATOM_GENRE2 20
#define ATOM_TEMPO 21
#define ATOM_COVER 22
#define ATOM_DRMS 23
#define ATOM_SINF 24
#define ATOM_SCHI 25

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#ifndef _WIN32
#define stricmp strcasecmp
#endif

/* file callback structure */
typedef struct
{
    uint32_t (*read)(void *user_data, void *buffer, uint32_t length);
    uint32_t (*write)(void *udata, void *buffer, uint32_t length);
    uint32_t (*seek)(void *user_data, uint64_t position);
	uint32_t (*truncate)(void *user_data);
    void *user_data;
} mp4ff_callback_t;


/* metadata tag structure */
typedef struct
{
    char *item;
    char *value;
} mp4ff_tag_t;

/* metadata list structure */
typedef struct
{
    mp4ff_tag_t *tags;
    uint32_t count;
} mp4ff_metadata_t;


typedef struct
{
    int32_t type;
    int32_t channelCount;
    int32_t sampleSize;
    uint16_t sampleRate;
	int32_t audioType;

    /* stsd */
    int32_t stsd_entry_count;

    /* stsz */
	int32_t stsz_sample_size;
	int32_t stsz_sample_count;
    int32_t *stsz_table;

    /* stts */
	int32_t stts_entry_count;
    int32_t *stts_sample_count;
    int32_t *stts_sample_delta;

    /* stsc */
	int32_t stsc_entry_count;
    int32_t *stsc_first_chunk;
    int32_t *stsc_samples_per_chunk;
    int32_t *stsc_sample_desc_index;

    /* stsc */
	int32_t stco_entry_count;
    int32_t *stco_chunk_offset;

	/* ctts */
	int32_t ctts_entry_count;
	int32_t *ctts_sample_count;
	int32_t *ctts_sample_offset;

    /* esde */
    uint8_t *decoderConfig;
    int32_t decoderConfigLen;

	uint32_t maxBitrate;
	uint32_t avgBitrate;
	
	uint32_t timeScale;
	uint64_t duration;

#ifdef ITUNES_DRM
    /* drms */
    void *p_drms;
#endif

} mp4ff_track_t;

/* mp4 main file structure */
typedef struct
{
    /* stream to read from */
	mp4ff_callback_t *stream;
    int64_t current_position;

    int32_t moov_read;
    uint64_t moov_offset;
    uint64_t moov_size;
    uint8_t last_atom;
    uint64_t file_size;

    /* mvhd */
    int32_t time_scale;
    int32_t duration;

    /* incremental track index while reading the file */
    int32_t total_tracks;

    /* track data */
    mp4ff_track_t *track[MAX_TRACKS];

    /* metadata */
    mp4ff_metadata_t tags;
} mp4ff_t;




/* mp4util.c */
int32_t mp4ff_read_data(mp4ff_t *f, int8_t *data, uint32_t size);
int32_t mp4ff_write_data(mp4ff_t *f, int8_t *data, uint32_t size);
uint64_t mp4ff_read_int64(mp4ff_t *f);
uint32_t mp4ff_read_int32(mp4ff_t *f);
uint32_t mp4ff_read_int24(mp4ff_t *f);
uint16_t mp4ff_read_int16(mp4ff_t *f);
uint8_t mp4ff_read_char(mp4ff_t *f);
int32_t mp4ff_write_int32(mp4ff_t *f,const uint32_t data);
uint32_t mp4ff_read_mp4_descr_length(mp4ff_t *f);
int64_t mp4ff_position(const mp4ff_t *f);
int32_t mp4ff_set_position(mp4ff_t *f, const int64_t position);
int32_t mp4ff_truncate(mp4ff_t * f);
char * mp4ff_read_string(mp4ff_t * f,uint32_t length);

/* mp4atom.c */
static int32_t mp4ff_atom_get_size(const int8_t *data);
static int32_t mp4ff_atom_compare(const int8_t a1, const int8_t b1, const int8_t c1, const int8_t d1,
                                  const int8_t a2, const int8_t b2, const int8_t c2, const int8_t d2);
static uint8_t mp4ff_atom_name_to_type(const int8_t a, const int8_t b, const int8_t c, const int8_t d);
uint64_t mp4ff_atom_read_header(mp4ff_t *f, uint8_t *atom_type, uint8_t *header_size);
static int32_t mp4ff_read_stsz(mp4ff_t *f);
static int32_t mp4ff_read_esds(mp4ff_t *f);
static int32_t mp4ff_read_mp4a(mp4ff_t *f);
static int32_t mp4ff_read_stsd(mp4ff_t *f);
static int32_t mp4ff_read_stsc(mp4ff_t *f);
static int32_t mp4ff_read_stco(mp4ff_t *f);
static int32_t mp4ff_read_stts(mp4ff_t *f);
#ifdef USE_TAGGING
static int32_t mp4ff_read_meta(mp4ff_t *f, const uint64_t size);
#endif
int32_t mp4ff_atom_read(mp4ff_t *f, const int32_t size, const uint8_t atom_type);

/* mp4sample.c */
static int32_t mp4ff_chunk_of_sample(const mp4ff_t *f, const int32_t track, const int32_t sample,
                                     int32_t *chunk_sample, int32_t *chunk);
static int32_t mp4ff_chunk_to_offset(const mp4ff_t *f, const int32_t track, const int32_t chunk);
static int32_t mp4ff_sample_range_size(const mp4ff_t *f, const int32_t track,
                                       const int32_t chunk_sample, const int32_t sample);
static int32_t mp4ff_sample_to_offset(const mp4ff_t *f, const int32_t track, const int32_t sample);
int32_t mp4ff_audio_frame_size(const mp4ff_t *f, const int32_t track, const int32_t sample);
int32_t mp4ff_set_sample_position(mp4ff_t *f, const int32_t track, const int32_t sample);

#ifdef USE_TAGGING
/* mp4meta.c */
static int32_t mp4ff_tag_add_field(mp4ff_metadata_t *tags, const char *item, const char *value);
static int32_t mp4ff_tag_set_field(mp4ff_metadata_t *tags, const char *item, const char *value);
static int32_t mp4ff_set_metadata_name(mp4ff_t *f, const uint8_t atom_type, char **name);
static int32_t mp4ff_parse_tag(mp4ff_t *f, const uint8_t parent_atom_type, const int32_t size);
static int32_t mp4ff_meta_find_by_name(const mp4ff_t *f, const char *item, char **value);
int32_t mp4ff_parse_metadata(mp4ff_t *f, const int32_t size);
int32_t mp4ff_tag_delete(mp4ff_metadata_t *tags);
int32_t mp4ff_meta_get_num_items(const mp4ff_t *f);
int32_t mp4ff_meta_get_by_index(const mp4ff_t *f, uint32_t index,
                            char **item, char **value);
int32_t mp4ff_meta_get_title(const mp4ff_t *f, char **value);
int32_t mp4ff_meta_get_artist(const mp4ff_t *f, char **value);
int32_t mp4ff_meta_get_writer(const mp4ff_t *f, char **value);
int32_t mp4ff_meta_get_album(const mp4ff_t *f, char **value);
int32_t mp4ff_meta_get_date(const mp4ff_t *f, char **value);
int32_t mp4ff_meta_get_tool(const mp4ff_t *f, char **value);
int32_t mp4ff_meta_get_comment(const mp4ff_t *f, char **value);
int32_t mp4ff_meta_get_genre(const mp4ff_t *f, char **value);
int32_t mp4ff_meta_get_track(const mp4ff_t *f, char **value);
int32_t mp4ff_meta_get_disc(const mp4ff_t *f, char **value);
int32_t mp4ff_meta_get_compilation(const mp4ff_t *f, char **value);
int32_t mp4ff_meta_get_tempo(const mp4ff_t *f, char **value);
int32_t mp4ff_meta_get_coverart(const mp4ff_t *f, char **value);
#endif

/* mp4ff.c */
mp4ff_t *mp4ff_open_read(mp4ff_callback_t *f);
#ifdef USE_TAGGING
mp4ff_t *mp4ff_open_edit(mp4ff_callback_t *f);
#endif
void mp4ff_close(mp4ff_t *ff);
void mp4ff_track_add(mp4ff_t *f);
int32_t parse_sub_atoms(mp4ff_t *f, const uint64_t total_size);
int32_t parse_atoms(mp4ff_t *f);

int32_t mp4ff_get_sample_duration(const mp4ff_t *f, const int32_t track, const int32_t sample);
int64_t mp4ff_get_sample_position(const mp4ff_t *f, const int32_t track, const int32_t sample);
int32_t mp4ff_get_sample_offset(const mp4ff_t *f, const int32_t track, const int32_t sample);
int32_t mp4ff_find_sample(const mp4ff_t *f, const int32_t track, const int64_t offset,int32_t * toskip);

int32_t mp4ff_read_sample(mp4ff_t *f, const int32_t track, const int32_t sample,
                          uint8_t **audio_buffer,  uint32_t *bytes);
int32_t mp4ff_get_decoder_config(const mp4ff_t *f, const int32_t track,
                                 uint8_t** ppBuf, uint32_t* pBufSize);
int32_t mp4ff_total_tracks(const mp4ff_t *f);
int32_t mp4ff_time_scale(const mp4ff_t *f, const int32_t track);
int32_t mp4ff_num_samples(const mp4ff_t *f, const int32_t track);

uint32_t mp4ff_meta_genre_to_index(const char * genrestr);//returns 1-based index, 0 if not found
const char * mp4ff_meta_index_to_genre(uint32_t idx);//returns pointer to static string


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif