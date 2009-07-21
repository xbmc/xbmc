/* test_libFLAC - Unit tester for libFLAC
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef FLAC__TEST_LIBS_COMMON_METADATA_UTILS_H
#define FLAC__TEST_LIBS_COMMON_METADATA_UTILS_H

/*
 * These are not tests, just utility functions used by the metadata tests
 */

#include "FLAC/format.h"

FLAC__bool mutils__compare_block_data_streaminfo(const FLAC__StreamMetadata_StreamInfo *block, const FLAC__StreamMetadata_StreamInfo *blockcopy);

FLAC__bool mutils__compare_block_data_padding(const FLAC__StreamMetadata_Padding *block, const FLAC__StreamMetadata_Padding *blockcopy, unsigned block_length);

FLAC__bool mutils__compare_block_data_application(const FLAC__StreamMetadata_Application *block, const FLAC__StreamMetadata_Application *blockcopy, unsigned block_length);

FLAC__bool mutils__compare_block_data_seektable(const FLAC__StreamMetadata_SeekTable *block, const FLAC__StreamMetadata_SeekTable *blockcopy);

FLAC__bool mutils__compare_block_data_vorbiscomment(const FLAC__StreamMetadata_VorbisComment *block, const FLAC__StreamMetadata_VorbisComment *blockcopy);

FLAC__bool mutils__compare_block_data_cuesheet(const FLAC__StreamMetadata_CueSheet *block, const FLAC__StreamMetadata_CueSheet *blockcopy);

FLAC__bool mutils__compare_block_data_picture(const FLAC__StreamMetadata_Picture *block, const FLAC__StreamMetadata_Picture *blockcopy);

FLAC__bool mutils__compare_block_data_unknown(const FLAC__StreamMetadata_Unknown *block, const FLAC__StreamMetadata_Unknown *blockcopy, unsigned block_length);

FLAC__bool mutils__compare_block(const FLAC__StreamMetadata *block, const FLAC__StreamMetadata *blockcopy);

void mutils__init_metadata_blocks(
	FLAC__StreamMetadata *streaminfo,
	FLAC__StreamMetadata *padding,
	FLAC__StreamMetadata *seektable,
	FLAC__StreamMetadata *application1,
	FLAC__StreamMetadata *application2,
	FLAC__StreamMetadata *vorbiscomment,
	FLAC__StreamMetadata *cuesheet,
	FLAC__StreamMetadata *picture,
	FLAC__StreamMetadata *unknown
);

void mutils__free_metadata_blocks(
	FLAC__StreamMetadata *streaminfo,
	FLAC__StreamMetadata *padding,
	FLAC__StreamMetadata *seektable,
	FLAC__StreamMetadata *application1,
	FLAC__StreamMetadata *application2,
	FLAC__StreamMetadata *vorbiscomment,
	FLAC__StreamMetadata *cuesheet,
	FLAC__StreamMetadata *picture,
	FLAC__StreamMetadata *unknown
);

#endif
