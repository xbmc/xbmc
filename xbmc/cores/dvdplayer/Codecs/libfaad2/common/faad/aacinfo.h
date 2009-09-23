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
** $Id: aacinfo.h,v 1.3 2003/07/29 08:20:11 menno Exp $
**/

#ifndef AACINFO_H__
#define AACINFO_H__

#include "filestream.h"

typedef struct {
    int version;
    int channels;
    int sampling_rate;
    int bitrate;
    int length;
    int object_type;
    int headertype;
} faadAACInfo;

int get_AAC_format(char *filename, faadAACInfo *info,
                   unsigned long **seek_table, int *seek_table_len,
                   int no_seek_table);

static int read_ADIF_header(FILE_STREAM *file, faadAACInfo *info);
static int read_ADTS_header(FILE_STREAM *file, faadAACInfo *info,
                            unsigned long **seek_table, int *seek_table_len,
                            int tagsize, int no_seek_table);
int StringComp(char const *str1, char const *str2, unsigned long len);

#endif
