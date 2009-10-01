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
** $Id: filestream.h,v 1.3 2003/07/29 08:20:11 menno Exp $
**/

#ifndef FILESTREAM_H
#define FILESTREAM_H

typedef struct {
    HANDLE stream;
    unsigned short inetStream;
    unsigned char *data;
    int http;
    int buffer_offset;
    int buffer_length;
    int file_offset;
    int http_file_length;
} FILE_STREAM;

extern long m_local_buffer_size;
extern long m_stream_buffer_size;

FILE_STREAM *open_filestream(char *filename);
int read_byte_filestream(FILE_STREAM *fs);
int read_buffer_filestream(FILE_STREAM *fs, void *data, int length);
unsigned long filelength_filestream(FILE_STREAM *fs);
void close_filestream(FILE_STREAM *fs);
void seek_filestream(FILE_STREAM *fs, unsigned long offset, int mode);
unsigned long tell_filestream(FILE_STREAM *fs);
int http_file_open(char *url, FILE_STREAM *fs);

int WinsockInit();
void WinsockDeInit();
void CloseTCP(int s);
#endif