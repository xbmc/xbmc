/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003 M. Bakker, Ahead Software AG, http://www.nero.com
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
** $Id: utils.c,v 1.6 2003/07/29 08:20:14 menno Exp $
**/

#define WIN32_LEAN_AND_MEAN
#ifndef _LINUX
#include <windows.h>
#endif

#ifndef __APPLE__
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <mp4ff.h>
#include <faad.h>
#include "utils.h"

int StringComp(char const *str1, char const *str2, unsigned long len)
{
  signed int c1 = 0, c2 = 0;

  while (len--)
  {
    c1 = tolower(*str1++);
    c2 = tolower(*str2++);

    if (c1 == 0 || c1 != c2)
      break;
  }

  return c1 - c2;
}

int GetAACTrack(mp4ff_t *infile)
{
    /* find AAC track */
    int i, rc;
    int numTracks = mp4ff_total_tracks(infile);
    int type;

    for (i = 0; i < numTracks; i++)
    {
        unsigned char *buff = NULL;
        int buff_size = 0;
        mp4AudioSpecificConfig mp4ASC;

        mp4ff_get_decoder_config(infile, i, &buff, &buff_size);

        if (buff)
        {
            rc = AudioSpecificConfig(buff, buff_size, &mp4ASC);
            free(buff);

            if (rc < 0)
                continue;
            return i;
        }
    }

    /* can't decode this */
    return -1;
}

char *convert3in4to3in3(void *sample_buffer, int samples)
{
  int i;
  long *sample_buffer24 = (long*)sample_buffer;
  char *data = malloc(samples*3*sizeof(char));

  for (i = 0; i < samples; i++)
  {
    data[i*3] = sample_buffer24[i] & 0xFF;
    data[i*3+1] = (sample_buffer24[i] >> 8) & 0xFF;
    data[i*3+2] = (sample_buffer24[i] >> 16) & 0xFF;
  }

  return data;
}
