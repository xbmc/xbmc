/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2005 M. Bakker, Nero AG, http://www.nero.com
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
** The "appropriate copyright message" mentioned in section 2c of the GPLv2
** must read: "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Nero AG through Mpeg4AAClicense@nero.com.
**
** $Id: mp4util.c,v 1.20 2007/11/01 12:33:29 menno Exp $
**/

#include "mp4ffint.h"
#include <stdlib.h>

int32_t mp4ff_read_data(mp4ff_t *f, int8_t *data, uint32_t size)
{
    int32_t result = 1;

    result = f->stream->read(f->stream->user_data, data, size);

    f->current_position += size;

    return result;
}

int32_t mp4ff_truncate(mp4ff_t * f)
{
	return f->stream->truncate(f->stream->user_data);
}

int32_t mp4ff_write_data(mp4ff_t *f, int8_t *data, uint32_t size)
{
    int32_t result = 1;

    result = f->stream->write(f->stream->user_data, data, size);

    f->current_position += size;

    return result;
}

int32_t mp4ff_write_int32(mp4ff_t *f,const uint32_t data)
{
	uint32_t result;
    uint32_t a, b, c, d;
    int8_t temp[4];
    
    *(uint32_t*)temp = data;
    a = (uint8_t)temp[0];
    b = (uint8_t)temp[1];
    c = (uint8_t)temp[2];
    d = (uint8_t)temp[3];

    result = (a<<24) | (b<<16) | (c<<8) | d;

    return mp4ff_write_data(f,(uint8_t*)&result,sizeof(result));
}

int32_t mp4ff_set_position(mp4ff_t *f, const int64_t position)
{
    f->stream->seek(f->stream->user_data, position);
    f->current_position = position;

    return 0;
}

int64_t mp4ff_position(const mp4ff_t *f)
{
    return f->current_position;
}

uint64_t mp4ff_read_int64(mp4ff_t *f)
{
    uint8_t data[8];
    uint64_t result = 0;
    int8_t i;

    mp4ff_read_data(f, data, 8);

    for (i = 0; i < 8; i++)
    {
        result |= ((uint64_t)data[i]) << ((7 - i) * 8);
    }

    return result;
}

uint32_t mp4ff_read_int32(mp4ff_t *f)
{
    uint32_t result;
    uint32_t a, b, c, d;
    int8_t data[4];
    
    mp4ff_read_data(f, data, 4);
    a = (uint8_t)data[0];
    b = (uint8_t)data[1];
    c = (uint8_t)data[2];
    d = (uint8_t)data[3];

    result = (a<<24) | (b<<16) | (c<<8) | d;
    return (uint32_t)result;
}

uint32_t mp4ff_read_int24(mp4ff_t *f)
{
    uint32_t result;
    uint32_t a, b, c;
    int8_t data[4];
    
    mp4ff_read_data(f, data, 3);
    a = (uint8_t)data[0];
    b = (uint8_t)data[1];
    c = (uint8_t)data[2];

    result = (a<<16) | (b<<8) | c;
    return (uint32_t)result;
}

uint16_t mp4ff_read_int16(mp4ff_t *f)
{
    uint32_t result;
    uint32_t a, b;
    int8_t data[2];
    
    mp4ff_read_data(f, data, 2);
    a = (uint8_t)data[0];
    b = (uint8_t)data[1];

    result = (a<<8) | b;
    return (uint16_t)result;
}

char * mp4ff_read_string(mp4ff_t * f,uint32_t length)
{
	char * str = (char*)malloc(length + 1);
	if (str!=0)
	{
		if ((uint32_t)mp4ff_read_data(f,str,length)!=length)
		{
			free(str);
			str = 0;
		}
		else
		{
			str[length] = 0;
		}
	}
	return str;	
}

uint8_t mp4ff_read_char(mp4ff_t *f)
{
    uint8_t output;
    mp4ff_read_data(f, &output, 1);
    return output;
}

uint32_t mp4ff_read_mp4_descr_length(mp4ff_t *f)
{
    uint8_t b;
    uint8_t numBytes = 0;
    uint32_t length = 0;

    do
    {
        b = mp4ff_read_char(f);
        numBytes++;
        length = (length << 7) | (b & 0x7F);
    } while ((b & 0x80) && numBytes < 4);

    return length;
}
