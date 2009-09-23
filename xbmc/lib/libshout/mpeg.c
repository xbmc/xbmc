/* mpeg.c - jonclegg@yahoo.com
 * various ripped off mpeg stuff, mainly for finding the first good header in a mp3 chunk
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
#include <string.h>
#include "mpeg.h"

#define MPG_MD_MONO 3

/**********************************************************************************
 * Public functions
 **********************************************************************************/
error_code	mpeg_find_first_header(const char* buffer, int size, int min_good_frames, int *frame_pos);
error_code	mpeg_find_last_header(const char *buffer, int size, int min_good_frames, int *frame_pos);

/**********************************************************************************
 * Private functions
 **********************************************************************************/
static int get_frame_size(mp3_header_t *mh);
static BOOL get_header(unsigned char *buff, mp3_header_t *mh);
static BOOL header_sane(mp3_header_t *mh);
static BOOL sameHeader(mp3_header_t *mh1, mp3_header_t *mh2);
static unsigned int decode_bitrate(mp3_header_t *mh);

static unsigned int bitrates[3][3][15] =
{
  {
    {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
    {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
    {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}
  },
  {
    {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
  },
  {
    {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
  }
};

static unsigned int s_freq_mult[3][4] = 
{
    {882, 960, 640, 0},
    {441, 480, 320, 0},
    {441, 480, 320, 0}
};				// From FreeAmp Xing decoder

/*
static char *mode_names[5] = {"stereo", "j-stereo", "dual-ch", "single-ch", "multi-ch"};
static char *layer_names[3] = {"I", "II", "III"};
static char *version_names[3] = {"MPEG-1", "MPEG-2 LSF", "MPEG-2.5"};
static char *version_nums[3] = {"1", "2", "2.5"};	
*/


/*
 * Finds the first header good for min_good_frames and fills header_pos with  the position 
 */
error_code mpeg_find_first_header(const char* pbuff, int size, int min_good_frames, int *header_pos)
{
	int i, n;
	unsigned char *buff = (unsigned char*)pbuff;
	unsigned char *buff2;
	mp3_header_t mh1, mh2;
	int frame_size;
	int remaining;

	for(i = 0; i < size; i++, buff++)
	{
		if (*buff == 255)
		{
			get_header(buff, &mh1);	
			if (!header_sane(&mh1))
				continue;

			frame_size = get_frame_size(&mh1);
			if (frame_size < 21)
				continue;

			buff2 = buff + frame_size;
			remaining = size - i - frame_size;
			for(n = 1; (n < min_good_frames) && (remaining >= 4); n++)
			{
				get_header(buff2, &mh2);
				if (!header_sane(&mh2)) break;
				if (!sameHeader(&mh1, &mh2)) break;

				remaining -= frame_size;
				buff2 += frame_size;
			}
			if (n == min_good_frames)
			{
				*header_pos = i;		
				return SR_SUCCESS;
			}
		}	
	}

	return SR_ERROR_CANT_FIND_MPEG_HEADER;
}

/*
 * Finds the last good frame within <buffer>, places that frame pos in <header_pos>
 */
error_code mpeg_find_last_header(const char *buffer, int size, int min_good_frames, int *header_pos)
{
	int frame_pos = 0;
	int cur = 0;


	// Keep getting frames till we run out of space
	while(mpeg_find_first_header(&buffer[cur], size-cur, min_good_frames, &frame_pos))
		cur += (frame_pos + 4);

	*header_pos = (cur-4);
	return SR_SUCCESS;
}
		

/*
 * Checks if the headers are the same
 */
BOOL sameHeader(mp3_header_t *mh1, mp3_header_t *mh2)
{
	if ((mh1->lay 			== mh2->lay) &&
	    (mh1->version 		== mh2->version) &&
	    (mh1->error_protection	== mh2->error_protection) &&
	    (mh1->bitrate_index		== mh2->bitrate_index) &&
	    (mh1->sampling_frequency	== mh2->sampling_frequency) &&
	//  (mh1->padding		== mh2->padding) &&
	    (mh1->extension		== mh2->extension) &&
	    (mh1->mode			== mh2->mode) &&
	    (mh1->mode_ext		== mh2->mode_ext) &&
	    (mh1->copyright		== mh2->copyright) &&
	    (mh1->original		== mh2->original) &&
	    (mh1->emphasis		== mh2->emphasis) &&
	    (mh1->stereo		== mh2->stereo))
		return TRUE;
	else
		return FALSE;
}            	

/*
 * Calcualtes the frame size based of a mp3_header
 */
int get_frame_size(mp3_header_t *mh)
{
	long mult = 0;

	if (decode_bitrate(mh) == 0)
		return 0;

	if (mh->lay == 1)
		return (240 * (decode_bitrate(mh) / 
	        	s_freq_mult[mh->version][mh->sampling_frequency])) * 4;
	else if (mh->lay == 2)
		mult = 2880;
	else if (mh->lay == 3)
	{
		if (mh->version == 0)
			mult = 2880;
		else if (mh->version == 1)
			mult = 1440;
		else if (mh->version == 2)	// MPEG 2.5	
			mult = 2880;
	}

	return mult * decode_bitrate(mh) / 
	       s_freq_mult[mh->version][mh->sampling_frequency] + mh->padding;
}

/* 
 * Extracts the mp3 header from <buff>
 */
BOOL get_header(unsigned char *buff, mp3_header_t *mh)
{
	unsigned char *buffer;
 	size_t 	temp;


	memset(mh, 0, sizeof(mh)); 
	buffer = buff;
	temp = ((buffer[0] << 4) & 0xFF0) | ((buffer[1] >> 4) & 0xE);
  
	if (temp != 0xFFE) 
	{
     		return FALSE;
	} 
	else 
	{
		switch ((buffer[1] >> 3 & 0x3))
		{
			case 3:
				mh->version = 0;
	  			break;
			case 2:
	  			mh->version = 1;
	  			break;
			case 0:
	  			mh->version = 2;
	  			break;
			default:
	  			return -1;
	  			break;
      		}
      		mh->lay = 4 - ((buffer[1] >> 1) & 0x3);
      		mh->error_protection = !(buffer[1] & 0x1);
      		mh->bitrate_index = (buffer[2] >> 4) & 0x0F;
      		mh->sampling_frequency = (buffer[2] >> 2) & 0x3;
      		mh->padding = (buffer[2] >> 1) & 0x01;
      		mh->extension = buffer[2] & 0x01;
      		mh->mode = (buffer[3] >> 6) & 0x3;
      		mh->mode_ext = (buffer[3] >> 4) & 0x03;
      		mh->copyright = (buffer[3] >> 3) & 0x01;
      		mh->original = (buffer[3] >> 2) & 0x1;
      		mh->emphasis = (buffer[3]) & 0x3;
      		mh->stereo = (mh->mode == MPG_MD_MONO) ? 1 : 2;

		return TRUE;
    }
}

unsigned int decode_bitrate(mp3_header_t *mh)
{
	return bitrates[mh->version][mh->lay-1][mh->bitrate_index];
}

/*
 * Checks if the header is even somewhat valid
 */
BOOL header_sane(mp3_header_t *mh)
{
	if ( ((mh->lay > 1) && (mh->lay < 4)) &&
	     ((mh->bitrate_index > 0) && (mh->bitrate_index < 15)) &&
	     ((mh->sampling_frequency >= 0) && (mh->sampling_frequency < 3)) &&
	     ((mh->version >= 0) && (mh->version < 3)))
		return TRUE;

	return FALSE;	
}

